#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"

namespace rv {

struct NavMeshBuildConfig
{
    float cellSize = 0.3f;
    float cellHeight = 0.2f;
    float agentHeight = 2.0f;
    float agentRadius = 0.5f;
    float agentMaxClimb = 0.25f;
    float agentMaxSlope = 45.0f;
    float regionMinSize = 8.0f;
    float regionMergeSize = 20.0f;
    float edgeMaxLen = 12.0f;
    float edgeMaxError = 1.3f;
    float vertsPerPoly = 6.0f;
    float detailSampleDist = 6.0f;
    float detailSampleMaxError = 1.0f;
    bool filterLowHangingObstacles = false;
};

class NavMeshBuildContext : public rcContext
{
public:
    NavMeshBuildContext();
    virtual ~NavMeshBuildContext() = default;

protected:
    virtual void doLog(const rcLogCategory category, const char* msg, const int len) override;
};

class NavMeshBuilder
{
public:
    static bool BuildSoloNavMesh(
        const std::vector<float>& vertices,
        const std::vector<int>& indices,
        const NavMeshBuildConfig& config,
        dtNavMesh*& outNavMesh,
        const std::vector<unsigned char>& triangleAreas = {});

    static void FreeNavMesh(dtNavMesh*& navMesh);
};

} // namespace rv
