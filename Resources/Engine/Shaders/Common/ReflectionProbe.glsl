#ifndef REFLECTION_PROBE_GLSL
#define REFLECTION_PROBE_GLSL

layout(binding = 28) uniform samplerCubeArray reflectionProbeArray;

struct ReflectionProbeData {
    vec4 position;     // xyz: center, w: intensity
    vec4 boxExtents;   // xyz: extents, w: blendDistance
    vec4 originOffset; // xyz: originOffset, w: priority
    vec4 params;       // x: boxProjection, y: interior, z: cubeIndex
};

#define MAX_REFLECTION_PROBES 8

layout(std140, binding = 2) uniform ReflectionProbeUBO {
    ReflectionProbeData u_ReflectionProbes[MAX_REFLECTION_PROBES];
    int u_ReflectionProbeCount;
    int _rpPad0;
    int _rpPad1;
    int _rpPad2;
};

vec3 EvaluateReflectionProbes(vec3 worldPos, vec3 N, vec3 V, float roughness, vec3 skyboxPrefiltered)
{
    if (u_ReflectionProbeCount <= 0)
    {
        return skyboxPrefiltered;
    }

    vec3 R = normalize(reflect(-V, N));
    float lod = roughness * 7.0;

    vec3 accumColor = vec3(0.0);
    float accumWeight = 0.0;
    float maxInterior = 0.0;

    for (int i = 0; i < u_ReflectionProbeCount; ++i)
    {
        float cubeIdx = u_ReflectionProbes[i].params.z;
        if (cubeIdx < 0.0) continue;

        vec3 pCenter = u_ReflectionProbes[i].position.xyz;
        float intensity = u_ReflectionProbes[i].position.w;
        vec3 boxExtents = u_ReflectionProbes[i].boxExtents.xyz;
        float blendDist = u_ReflectionProbes[i].boxExtents.w;
        vec3 originOffset = u_ReflectionProbes[i].originOffset.xyz;
        float priority = u_ReflectionProbes[i].originOffset.w;
        bool boxProj = (u_ReflectionProbes[i].params.x > 0.5);
        bool interior = (u_ReflectionProbes[i].params.y > 0.5);

        vec3 localPos = worldPos - pCenter;

        vec3 absLocal = abs(localPos);
        if (absLocal.x > boxExtents.x || absLocal.y > boxExtents.y || absLocal.z > boxExtents.z)
        {
            continue;
        }

        float weight = 1.0;
        if (blendDist > 0.001)
        {
            vec3 axisBlendDist = min(vec3(blendDist), boxExtents);
            vec3 blendAxes = clamp((boxExtents - absLocal) / axisBlendDist, 0.0, 1.0);
            weight = blendAxes.x * blendAxes.y * blendAxes.z;
        }

        if (weight <= 0.0001) continue;

        weight *= (1.0 + max(priority, 0.0) * 4.0);

        vec3 sampleDir = R;
        if (boxProj)
        {
            vec3 nrdir = normalize(R);
            vec3 dirSign = sign(nrdir);
            vec3 safeDir = mix(nrdir, dirSign * 0.00001, equal(nrdir, vec3(0.0)));

            // Clamp local pos strictly inside box so boundary rays never self ntersect or collapse to 0
            vec3 clampedLocalPos = clamp(localPos, -boxExtents * 0.999, boxExtents * 0.999);

            vec3 rbmax = (boxExtents - clampedLocalPos) / safeDir;
            vec3 rbmin = (-boxExtents - clampedLocalPos) / safeDir;
            vec3 rbminmax = mix(rbmin, rbmax, greaterThan(safeDir, vec3(0.0)));
            float fa = min(min(rbminmax.x, rbminmax.y), rbminmax.z);

            vec3 posOnBox = clampedLocalPos + safeDir * fa;
            sampleDir = posOnBox - originOffset;
        }

        vec3 probeColor = textureLod(reflectionProbeArray, vec4(sampleDir, cubeIdx), lod).rgb * intensity;

        accumColor += probeColor * weight;
        accumWeight += weight;

        if (interior)
        {
            maxInterior = max(maxInterior, weight);
        }
    }

    if (accumWeight > 0.0001)
    {
        vec3 probeSpecular = accumColor / accumWeight;
        float blendFactor = clamp(accumWeight, 0.0, 1.0);

        vec3 skyContrib = skyboxPrefiltered * (1.0 - maxInterior);
        return mix(skyContrib, probeSpecular, blendFactor);
    }

    return skyboxPrefiltered;
}

#endif
