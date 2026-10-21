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
in vec2 TexCoords;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_LowMip;

uniform float u_Intensity = 1.0;
uniform float u_Radius = 1.0;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_LowMip, 0));

    vec2 uvC = clamp(TexCoords, vec2(0.0), vec2(1.0));
    vec2 uvR = clamp(TexCoords + vec2(texelSize.x, 0.0) * u_Radius, vec2(0.0), vec2(1.0));
    vec2 uvL = clamp(TexCoords - vec2(texelSize.x, 0.0) * u_Radius, vec2(0.0), vec2(1.0));
    vec2 uvT = clamp(TexCoords + vec2(0.0, texelSize.y) * u_Radius, vec2(0.0), vec2(1.0));
    vec2 uvB = clamp(TexCoords - vec2(0.0, texelSize.y) * u_Radius, vec2(0.0), vec2(1.0));

    vec3 up = vec3(0.0);
    up += texture(u_LowMip, uvC).rgb * 4.0;
    up += texture(u_LowMip, uvR).rgb;
    up += texture(u_LowMip, uvL).rgb;
    up += texture(u_LowMip, uvT).rgb;
    up += texture(u_LowMip, uvB).rgb;

    up /= 8.0;

    FragColor = vec4(up * u_Intensity, 1.0);
}
