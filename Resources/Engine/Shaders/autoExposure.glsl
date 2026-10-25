#shader compute
#version 460 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "Common/Color.glsl"

layout(binding = 0) uniform sampler2D u_ScreenTexture;

uniform float u_DeltaTime;
uniform float u_MinExposure;
uniform float u_MaxExposure;
uniform float u_SpeedUp;
uniform float u_SpeedDown;
uniform float u_TargetLuminance;
uniform bool  u_Reset;

layout(std430, binding = 0) buffer AutoExposureBuffer {
    float b_Exposure;
    float b_AvgLuminance;
    float b_TargetExposure;
    float b_Padding;
};

shared float s_Lum[256];

void main()
{
    uint localIndex = gl_LocalInvocationIndex;

    float threadLogLumSum = 0.0;

    for (uint i = 0u; i < 4u; ++i)
    {
        uint sampleIdx = localIndex + i * 256u;
        uint sampleX = sampleIdx % 32u;
        uint sampleY = sampleIdx / 32u;

        vec2 uv = (vec2(sampleX, sampleY) + vec2(0.5)) / 32.0;
        vec3 color = texture(u_ScreenTexture, uv).rgb;

        float lum = Luminance(color);
        if (isnan(lum) || isinf(lum))
            lum = 0.0;

        threadLogLumSum += log(max(lum, 0.0001));
    }

    s_Lum[localIndex] = threadLogLumSum;
    barrier();

    for (uint s = 128u; s > 0u; s >>= 1u)
    {
        if (localIndex < s)
        {
            s_Lum[localIndex] += s_Lum[localIndex + s];
        }
        barrier();
    }

    if (localIndex == 0u)
    {
        float avgLogLum = s_Lum[0] / 1024.0;
        float currentLum = exp(avgLogLum);
        if (isnan(currentLum) || isinf(currentLum))
            currentLum = u_TargetLuminance;

        float prevLum = b_AvgLuminance;
        if (u_Reset || prevLum <= 0.00001 || isnan(prevLum) || isinf(prevLum))
        {
            prevLum = currentLum;
        }

        float speed = (currentLum > prevLum) ? u_SpeedUp : u_SpeedDown;
        float factor = 1.0 - exp(-clamp(u_DeltaTime, 0.0, 0.1) * speed);
        float adaptedLum = prevLum + (currentLum - prevLum) * factor;

        float targetExp = u_TargetLuminance / max(adaptedLum, 0.0001);
        float clampedExp = clamp(targetExp, u_MinExposure, u_MaxExposure);

        b_Exposure = clampedExp;
        b_AvgLuminance = adaptedLum;
        b_TargetExposure = targetExp;
        b_Padding = 0.0;
    }
}
