#shader vertex
#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

#shader fragment
#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"
#include "Common/Tonemapping.glsl"
#include "Common/PostProcess.glsl"

layout(binding = 0) uniform sampler2D screenTexture;
layout(binding = 1) uniform sampler2D bloomTexture;
layout(binding = 2) uniform sampler2D ssrTexture;
layout(binding = 3) uniform sampler2D depthTexture;

layout(std430, binding = 0) readonly buffer AutoExposureBuffer {
    float b_Exposure;
    float b_AvgLuminance;
    float b_TargetExposure;
    float b_Padding;
};

uniform bool u_AutoExposure;
uniform float bloomIntensity;
uniform float exposure;
uniform float brightness;
uniform float contrast;
uniform float saturation;
uniform float chromaticStrength;
uniform float vignetteIntensity;
uniform float vignetteSmoothness;

uniform bool u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogDensity;
uniform float u_FogStart;
uniform float u_FogEnd;
uniform int u_FogMode;

void main()
{
    vec3 hdrColor   = ChromaticAberration(screenTexture, TexCoords, chromaticStrength);
    vec3 bloomColor = texture(bloomTexture, TexCoords).rgb;

    if (u_FogEnabled)
    {
        float depth = texture(depthTexture, TexCoords).r;
        if (depth < 0.99999)
        {
            vec3 viewPos = ReconstructViewPos(TexCoords, depth, invProjection);
            float dist = length(viewPos);

            float fogFactor = 0.0;
            if (u_FogMode == 0) // Linear
            {
                fogFactor = (dist - u_FogStart) / max(u_FogEnd - u_FogStart, 0.001);
            }
            else if (u_FogMode == 1) // Exponential
            {
                float d = max(dist - u_FogStart, 0.0);
                fogFactor = 1.0 - exp(-d * u_FogDensity);
            }
            else // Exponential Squared
            {
                float d = max(dist - u_FogStart, 0.0);
                float expTerm = d * u_FogDensity;
                fogFactor = 1.0 - exp(-(expTerm * expTerm));
            }

            fogFactor = clamp(fogFactor, 0.0, 1.0);
            hdrColor = mix(hdrColor, u_FogColor, fogFactor);
        }
    }

    vec3 combined = hdrColor + bloomColor * bloomIntensity;

    float finalExposure = exposure;
    if (u_AutoExposure)
    {
        finalExposure *= b_Exposure;
    }

    vec3 mapped = ACESFilm(combined * finalExposure);

    mapped = LinearToSRGB(mapped);

    mapped = AdjustColor(mapped, brightness, contrast, saturation);

    mapped *= Vignette(TexCoords, vignetteIntensity, vignetteSmoothness);

    FragColor = vec4(mapped, 1.0);
}
