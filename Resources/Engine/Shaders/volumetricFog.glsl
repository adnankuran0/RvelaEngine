#shader vertex
#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

#shader fragment
#version 460 core

in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"
#include "Common/Shadows.glsl"
#include "Common/Dither.glsl"

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2DArray u_DirectionalShadowMap;

uniform float u_FogDensity;
uniform float u_Extinction;
uniform float u_Anisotropy;
uniform float u_MaxDistance;
uniform float u_HeightFalloff;
uniform float u_BaseHeight;
uniform float u_Intensity;
uniform float u_AmbientIntensity;
uniform vec3 u_FogColor;
uniform int u_StepCount;

float HenyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 - g2) / (4.0 * 3.14159265359 * pow(max(denom, 0.0001), 1.5));
}

float GetFogDensity(vec3 pos)
{
    float height = pos.y - u_BaseHeight;
    float factor = exp(-max(height, 0.0) * u_HeightFalloff);
    return u_FogDensity * factor;
}

void main()
{
    float depth = texture(gDepth, TexCoords).r;

    vec3 viewFar = ReconstructViewPos(TexCoords, 1.0, invProjection);
    vec3 worldFar = (invView * vec4(viewFar, 1.0)).xyz;
    vec3 rayDir = normalize(worldFar - camPos);

    float rayLength = u_MaxDistance;
    if (depth < 0.999999)
    {
        vec3 viewPos = ReconstructViewPos(TexCoords, depth, invProjection);
        vec3 surfaceWorldPos = (invView * vec4(viewPos, 1.0)).xyz;
        rayLength = min(length(surfaceWorldPos - camPos), u_MaxDistance);
    }

    if (rayLength <= 0.001)
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    int steps = clamp(u_StepCount, 4, 64);
    float stepSize = rayLength / float(steps);

    float dither = GetDitherThreshold(gl_FragCoord.xy);

    vec3 accumScattering = vec3(0.0);
    float transmittance = 1.0;

    vec3 lightDir = (hasDirectionalLight == 1) ? normalize(directionalLight.direction.xyz) : vec3(0.0, -1.0, 0.0);
    float cosTheta = dot(rayDir, -lightDir);
    float phase = HenyeyGreenstein(cosTheta, u_Anisotropy);

    vec3 sunColor = (hasDirectionalLight == 1) ? (directionalLight.colorIntensity.rgb * directionalLight.colorIntensity.w) : vec3(0.0);
    vec3 ambientLight = u_FogColor * u_AmbientIntensity;

    for (int i = 0; i < steps; ++i)
    {
        float t = (float(i) + dither) * stepSize;
        if (t >= rayLength) break;

        vec3 samplePos = camPos + rayDir * t;
        float density = GetFogDensity(samplePos);

        if (density > 0.00001)
        {
            float shadow = 0.0;
            if (hasDirectionalLight == 1 && directionalLight.direction.w > 0.5)
            {
                float sampleViewDepth = -((view * vec4(samplePos, 1.0)).z);
                shadow = calculateDirectionalShadow(u_DirectionalShadowMap, samplePos, vec3(0.0), -lightDir, sampleViewDepth, 0.001, 0.0, 0.0);
            }

            float lightVisibility = 1.0 - shadow;
            vec3 stepLight = sunColor * lightVisibility * phase + ambientLight;

            float stepExtinction = density * (u_Extinction + 0.0001);
            vec3 stepScattering = density * stepLight * u_FogColor;

            float stepTransmittance = exp(-stepExtinction * stepSize);
            vec3 integScattering = (stepScattering - stepScattering * stepTransmittance) / stepExtinction;

            accumScattering += transmittance * integScattering;
            transmittance *= stepTransmittance;

            if (transmittance < 0.005)
                break;
        }
    }

    FragColor = vec4(accumScattering * u_Intensity, transmittance);
}
