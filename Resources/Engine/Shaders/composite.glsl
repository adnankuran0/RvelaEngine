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

#include "Common/Tonemapping.glsl"
#include "Common/PostProcess.glsl"

layout(binding = 0) uniform sampler2D screenTexture;
layout(binding = 1) uniform sampler2D bloomTexture;
layout(binding = 2) uniform sampler2D ssrTexture;

uniform float bloomIntensity;
uniform float exposure;
uniform float chromaticStrength;
uniform float vignetteIntensity;
uniform float vignetteSmoothness;

void main()
{
    vec3 hdrColor   = ChromaticAberration(screenTexture, TexCoords, chromaticStrength);
    vec3 bloomColor = texture(bloomTexture, TexCoords).rgb;
    vec4 ssrSample  = texture(ssrTexture,  TexCoords);

    hdrColor = mix(hdrColor, hdrColor + ssrSample.rgb, ssrSample.a);

    vec3 combined = hdrColor + bloomColor * bloomIntensity;

    combined    = vec3(1.0) - exp(-combined * exposure);
    vec3 mapped = ACESFilm(combined);

    mapped = LinearToSRGB(mapped);

    mapped *= Vignette(TexCoords, vignetteIntensity, vignetteSmoothness);

    FragColor = vec4(mapped, 1.0);
}