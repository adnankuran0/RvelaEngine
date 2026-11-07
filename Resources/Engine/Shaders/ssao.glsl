#shader vertex
#version 460 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment
#version 460 core
out float FragColor;
in vec2 TexCoords;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"

layout(binding = 0) uniform sampler2D gNormal;
layout(binding = 1) uniform sampler2D gDepth;

uniform float radius = 1.0;      
uniform float bias = 0.025;
uniform float intensity = 1.0;
uniform vec2 u_TargetResolution;

const float PI = 3.14159265358979323846;
const float HALF_PI = 1.57079632679489661923;

// Fast polynomial inverse cosine approximation (XeGTAO / Lagarde)
float FastACos(float inX)
{
    float x = abs(inX);
    float res = -0.156583 * x + HALF_PI;
    res *= sqrt(max(0.0, 1.0 - x));
    return (inX >= 0.0) ? res : PI - res;
}

float Bayer2x2(vec2 screenPos)
{
    ivec2 p = ivec2(screenPos) % 2;
    const float bayer[4] = float[4](
        0.0 / 4.0, 2.0 / 4.0,
        3.0 / 4.0, 1.0 / 4.0
    );
    return bayer[p.x + p.y * 2];
}

void main()
{
    float depth = texture(gDepth, TexCoords).r;
    if (depth >= 0.999999)
    {
        FragColor = 1.0;
        return;
    }

    vec3 fragPos = ReconstructViewPos(TexCoords, depth, invProjection);
    vec3 viewDir = -normalize(fragPos);

    vec3 dX = dFdx(fragPos);
    vec3 dY = dFdy(fragPos);
    vec3 geomNormal = cross(dX, dY);
    float gLenSq = dot(geomNormal, geomNormal);
    if (gLenSq > 1e-8)
    {
        geomNormal = normalize(geomNormal);
        if (dot(geomNormal, viewDir) < 0.0)
            geomNormal = -geomNormal;
    }
    else
    {
        geomNormal = normalize(texture(gNormal, TexCoords).rgb);
    }

    float maxScreenRadius = 0.25;
    float radiusUV = (radius * projection[1][1]) / (2.0 * max(-fragPos.z, 0.1));
    radiusUV = clamp(radiusUV, 8.0 / u_TargetResolution.y, maxScreenRadius);

    float minStepUV = 3.5 / u_TargetResolution.y;

    vec2 aspectScale = vec2(1.0 / (windowSize.x / windowSize.y), 1.0);

    float bayerVal = Bayer2x2(gl_FragCoord.xy);
    float stepJitter = (bayerVal - 0.5) * 0.5;

    const int NUM_SLICES = 3;
    const int NUM_STEPS = 3;

    float sliceDither = (bayerVal - 0.5) * (PI / float(NUM_SLICES));

    float effectiveBias = max(bias, 0.04);

    float totalVisibility = 0.0;

    for (int s = 0; s < NUM_SLICES; ++s)
    {
        float sliceAngle = float(s) * (PI / float(NUM_SLICES)) + sliceDither;
        vec2 dir2D = vec2(cos(sliceAngle), sin(sliceAngle));
        vec2 dirUV = dir2D * aspectScale;

        vec3 sliceDir = vec3(dir2D, 0.0);

        vec3 orthoDir = sliceDir - dot(sliceDir, viewDir) * viewDir;
        vec3 axisVec = normalize(cross(orthoDir, viewDir));
        vec3 projNormal = geomNormal - axisVec * dot(geomNormal, axisVec);
        float projNormalLen = length(projNormal);

        if (projNormalLen < 0.001)
        {
            totalVisibility += 1.0;
            continue;
        }

        projNormal /= projNormalLen;

        float signNorm = sign(dot(orthoDir, projNormal));
        float cosNorm = clamp(dot(projNormal, viewDir), 0.0, 1.0);
        float n = signNorm * FastACos(cosNorm);

        float lowHorizonCos0 = cos(n + HALF_PI);
        float lowHorizonCos1 = cos(n - HALF_PI);

        float horizonCos0 = lowHorizonCos0;
        float horizonCos1 = lowHorizonCos1;

        for (int step = 0; step < NUM_STEPS; ++step)
        {
            float stepFrac = (float(step) + 0.5 + stepJitter) / float(NUM_STEPS);
            float sDist = minStepUV + (radiusUV - minStepUV) * (stepFrac * stepFrac);

            vec2 sampleOffset = sDist * dirUV;

            vec2 sampleUV0 = TexCoords + sampleOffset;
            if (sampleUV0.x >= 0.0 && sampleUV0.x <= 1.0 && sampleUV0.y >= 0.0 && sampleUV0.y <= 1.0)
            {
                float sampleDepth0 = texture(gDepth, sampleUV0).r;
                if (sampleDepth0 < 0.999999)
                {
                    vec3 samplePos0 = ReconstructViewPos(sampleUV0, sampleDepth0, invProjection);
                    vec3 delta0 = samplePos0 - fragPos;
                    float dist0 = length(delta0);
                    if (dist0 > 0.01 && dist0 <= radius)
                    {
                        float nDotD0 = dot(geomNormal, delta0) / dist0;
                        if (nDotD0 > effectiveBias)
                        {
                            vec3 horizonVec0 = delta0 / dist0;
                            float shc0 = dot(horizonVec0, viewDir);
                            float distFactor0 = dist0 / radius;
                            float weight0 = clamp(1.0 - distFactor0 * distFactor0, 0.0, 1.0);
                            weight0 *= weight0;
                            shc0 = mix(lowHorizonCos0, shc0, weight0);
                            horizonCos0 = max(horizonCos0, shc0);
                        }
                    }
                }
            }

            vec2 sampleUV1 = TexCoords - sampleOffset;
            if (sampleUV1.x >= 0.0 && sampleUV1.x <= 1.0 && sampleUV1.y >= 0.0 && sampleUV1.y <= 1.0)
            {
                float sampleDepth1 = texture(gDepth, sampleUV1).r;
                if (sampleDepth1 < 0.999999)
                {
                    vec3 samplePos1 = ReconstructViewPos(sampleUV1, sampleDepth1, invProjection);
                    vec3 delta1 = samplePos1 - fragPos;
                    float dist1 = length(delta1);
                    if (dist1 > 0.01 && dist1 <= radius)
                    {
                        float nDotD1 = dot(geomNormal, delta1) / dist1;
                        if (nDotD1 > effectiveBias)
                        {
                            vec3 horizonVec1 = delta1 / dist1;
                            float shc1 = dot(horizonVec1, viewDir);
                            float distFactor1 = dist1 / radius;
                            float weight1 = clamp(1.0 - distFactor1 * distFactor1, 0.0, 1.0);
                            weight1 *= weight1;
                            shc1 = mix(lowHorizonCos1, shc1, weight1);
                            horizonCos1 = max(horizonCos1, shc1);
                        }
                    }
                }
            }
        }

        float h0 = -FastACos(clamp(horizonCos1, -1.0, 1.0));
        float h1 =  FastACos(clamp(horizonCos0, -1.0, 1.0));

        // Analytical closedform GTAO arc integration (Jimenez et al.)
        float iarc0 = (cosNorm + 2.0 * h0 * sin(n) - cos(2.0 * h0 - n)) * 0.25;
        float iarc1 = (cosNorm + 2.0 * h1 * sin(n) - cos(2.0 * h1 - n)) * 0.25;

        float unoccludedArc = cosNorm + n * sin(n);
        float sliceVisibility = clamp((iarc0 + iarc1) / max(unoccludedArc, 0.001), 0.0, 1.0);

        totalVisibility += sliceVisibility;
    }

    totalVisibility /= float(NUM_SLICES);
    float ao = clamp(totalVisibility, 0.0, 1.0);

    ao = pow(ao, intensity);

    FragColor = ao;
}
