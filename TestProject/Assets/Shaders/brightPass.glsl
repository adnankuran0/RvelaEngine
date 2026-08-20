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

out vec4 FragColor;
in vec2 TexCoords;

#include "Common/Color.glsl"

layout(binding = 0) uniform sampler2D hdrTexture;
uniform float threshold;
uniform float knee;

void main()
{
    vec3 color = texture(hdrTexture, TexCoords).rgb;
    float br = Luminance(color);

    float soft = clamp((br - threshold + knee) / (2.0 * knee + 0.0001), 0.0, 1.0);
    float contrib = max(br - threshold, 0.0) + soft * soft * knee;

    vec3 extracted = color * (contrib / max(br, 1e-4));

    float lum = Luminance(extracted);
    extracted *= 1.0 / (1.0 + lum);

    FragColor = vec4(extracted, 1.0);
}