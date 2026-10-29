#include "rvelapch.h"
#include "NavMeshBuilder.h"
#include <cmath>
#include <cstring>

namespace rv {

NavMeshBuildContext::NavMeshBuildContext()
    : rcContext(false)
{
}

void NavMeshBuildContext::doLog(const rcLogCategory category, const char* msg, const int len)
{
    (void)len;
    switch (category)
    {
    case RC_LOG_WARNING:
        LOG_WARN("[NavMesh] {}", msg);
        break;
    case RC_LOG_ERROR:
        LOG_ERROR("[NavMesh] {}", msg);
        break;
    default:
        break;
    }
}

bool NavMeshBuilder::BuildSoloNavMesh(
    const std::vector<float>& vertices,
    const std::vector<int>& indices,
    const NavMeshBuildConfig& config,
    dtNavMesh*& outNavMesh,
    const std::vector<unsigned char>& triangleAreas)
{
    outNavMesh = nullptr;

    if (vertices.empty() || indices.empty())
    {
        LOG_WARN("[NavMesh] No geometry provided to build navigation mesh.");
        return false;
    }

    const float* verts = vertices.data();
    const int numVerts = static_cast<int>(vertices.size()) / 3;
    const int* tris = indices.data();
    const int numTris = static_cast<int>(indices.size()) / 3;

    NavMeshBuildContext ctx;

    rcConfig rcCfg{};
    rcCfg.cs = config.cellSize;
    rcCfg.ch = config.cellHeight;
    rcCfg.walkableSlopeAngle = config.agentMaxSlope;
    rcCfg.walkableHeight = static_cast<int>(ceilf(config.agentHeight / rcCfg.ch));
    rcCfg.walkableClimb = static_cast<int>(floorf(config.agentMaxClimb / rcCfg.ch));
    rcCfg.walkableRadius = static_cast<int>(ceilf(config.agentRadius / rcCfg.cs));
    rcCfg.maxEdgeLen = static_cast<int>(config.edgeMaxLen / rcCfg.cs);
    rcCfg.maxSimplificationError = config.edgeMaxError;
    rcCfg.minRegionArea = static_cast<int>(rcSqr(config.regionMinSize));
    rcCfg.mergeRegionArea = static_cast<int>(rcSqr(config.regionMergeSize));
    rcCfg.maxVertsPerPoly = static_cast<int>(config.vertsPerPoly);
    rcCfg.detailSampleDist = config.detailSampleDist < 0.9f ? 0.0f : rcCfg.cs * config.detailSampleDist;
    rcCfg.detailSampleMaxError = rcCfg.ch * config.detailSampleMaxError;

    rcCalcBounds(verts, numVerts, rcCfg.bmin, rcCfg.bmax);
    rcCalcGridSize(rcCfg.bmin, rcCfg.bmax, rcCfg.cs, &rcCfg.width, &rcCfg.height);

    if (rcCfg.width <= 0 || rcCfg.height <= 0)
    {
        LOG_ERROR("[NavMesh] Calculated grid size is invalid: {}x{}", rcCfg.width, rcCfg.height);
        return false;
    }

    rcHeightfield* solid = rcAllocHeightfield();
    if (!solid)
    {
        LOG_ERROR("[NavMesh] Out of memory: 'solid' heightfield");
        return false;
    }

    if (!rcCreateHeightfield(&ctx, *solid, rcCfg.width, rcCfg.height, rcCfg.bmin, rcCfg.bmax, rcCfg.cs, rcCfg.ch))
    {
        LOG_ERROR("[NavMesh] Failed to create heightfield");
        rcFreeHeightField(solid);
        return false;
    }

    std::vector<unsigned char> triAreas(numTris, 0);
    if (!triangleAreas.empty() && triangleAreas.size() == static_cast<size_t>(numTris))
    {
        triAreas = triangleAreas;
        rcClearUnwalkableTriangles(&ctx, rcCfg.walkableSlopeAngle, verts, numVerts, tris, numTris, triAreas.data());
    }
    else
    {
        rcMarkWalkableTriangles(&ctx, rcCfg.walkableSlopeAngle, verts, numVerts, tris, numTris, triAreas.data());
    }

    if (!rcRasterizeTriangles(&ctx, verts, numVerts, tris, triAreas.data(), numTris, *solid, rcCfg.walkableClimb))
    {
        LOG_ERROR("[NavMesh] Failed to rasterize triangles");
        rcFreeHeightField(solid);
        return false;
    }

    if (config.filterLowHangingObstacles)
    {
        rcFilterLowHangingWalkableObstacles(&ctx, rcCfg.walkableClimb, *solid);
    }
    rcFilterLedgeSpans(&ctx, rcCfg.walkableHeight, rcCfg.walkableClimb, *solid);
    rcFilterWalkableLowHeightSpans(&ctx, rcCfg.walkableHeight, *solid);

    rcCompactHeightfield* chf = rcAllocCompactHeightfield();
    if (!chf)
    {
        LOG_ERROR("[NavMesh] Out of memory: compact heightfield");
        rcFreeHeightField(solid);
        return false;
    }

    if (!rcBuildCompactHeightfield(&ctx, rcCfg.walkableHeight, rcCfg.walkableClimb, *solid, *chf))
    {
        LOG_ERROR("[NavMesh] Failed to build compact heightfield");
        rcFreeHeightField(solid);
        rcFreeCompactHeightfield(chf);
        return false;
    }
    rcFreeHeightField(solid);

    if (!rcErodeWalkableArea(&ctx, rcCfg.walkableRadius, *chf))
    {
        LOG_ERROR("[NavMesh] Failed to erode walkable area");
        rcFreeCompactHeightfield(chf);
        return false;
    }

    if (!rcBuildDistanceField(&ctx, *chf))
    {
        LOG_ERROR("[NavMesh] Failed to build distance field");
        rcFreeCompactHeightfield(chf);
        return false;
    }

    if (!rcBuildRegions(&ctx, *chf, 0, rcCfg.minRegionArea, rcCfg.mergeRegionArea))
    {
        LOG_ERROR("[NavMesh] Failed to build regions");
        rcFreeCompactHeightfield(chf);
        return false;
    }

    rcContourSet* cset = rcAllocContourSet();
    if (!cset)
    {
        LOG_ERROR("[NavMesh] Out of memory: contour set");
        rcFreeCompactHeightfield(chf);
        return false;
    }

    if (!rcBuildContours(&ctx, *chf, rcCfg.maxSimplificationError, rcCfg.maxEdgeLen, *cset))
    {
        LOG_ERROR("[NavMesh] Failed to build contours");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        return false;
    }

    rcPolyMesh* pmesh = rcAllocPolyMesh();
    if (!pmesh)
    {
        LOG_ERROR("[NavMesh] Out of memory: poly mesh");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        return false;
    }

    if (!rcBuildPolyMesh(&ctx, *cset, rcCfg.maxVertsPerPoly, *pmesh))
    {
        LOG_ERROR("[NavMesh] Failed to build poly mesh");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        rcFreePolyMesh(pmesh);
        return false;
    }

    rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
    if (!dmesh)
    {
        LOG_ERROR("[NavMesh] Out of memory: poly mesh detail");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        rcFreePolyMesh(pmesh);
        return false;
    }

    if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, rcCfg.detailSampleDist, rcCfg.detailSampleMaxError, *dmesh))
    {
        LOG_ERROR("[NavMesh] Failed to build poly mesh detail");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return false;
    }

    rcFreeCompactHeightfield(chf);
    rcFreeContourSet(cset);

    for (int i = 0; i < pmesh->npolys; ++i)
    {
        if (pmesh->areas[i] == RC_WALKABLE_AREA)
        {
            pmesh->areas[i] = 1;
            pmesh->flags[i] = 1;
        }
    }

    dtNavMeshCreateParams params{};
    params.verts = pmesh->verts;
    params.vertCount = pmesh->nverts;
    params.polys = pmesh->polys;
    params.polyAreas = pmesh->areas;
    params.polyFlags = pmesh->flags;
    params.polyCount = pmesh->npolys;
    params.nvp = pmesh->nvp;
    params.detailMeshes = dmesh->meshes;
    params.detailVerts = dmesh->verts;
    params.detailVertsCount = dmesh->nverts;
    params.detailTris = dmesh->tris;
    params.detailTriCount = dmesh->ntris;
    params.walkableHeight = config.agentHeight;
    params.walkableRadius = config.agentRadius;
    params.walkableClimb = config.agentMaxClimb;
    rcVcopy(params.bmin, pmesh->bmin);
    rcVcopy(params.bmax, pmesh->bmax);
    params.cs = rcCfg.cs;
    params.ch = rcCfg.ch;
    params.buildBvTree = true;

    unsigned char* navData = nullptr;
    int navDataSize = 0;
    if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
    {
        LOG_ERROR("[NavMesh] Could not build Detour navmesh data");
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return false;
    }

    rcFreePolyMesh(pmesh);
    rcFreePolyMeshDetail(dmesh);

    dtNavMesh* navMesh = dtAllocNavMesh();
    if (!navMesh)
    {
        dtFree(navData);
        LOG_ERROR("[NavMesh] Could not allocate Detour navmesh");
        return false;
    }

    dtStatus status = navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
    if (dtStatusFailed(status))
    {
        dtFree(navData);
        dtFreeNavMesh(navMesh);
        LOG_ERROR("[NavMesh] Could not initialize Detour navmesh (status 0x{:x})", status);
        return false;
    }

    outNavMesh = navMesh;
    return true;
}

void NavMeshBuilder::FreeNavMesh(dtNavMesh*& navMesh)
{
    if (navMesh)
    {
        dtFreeNavMesh(navMesh);
        navMesh = nullptr;
    }
}

} // namespace rv
