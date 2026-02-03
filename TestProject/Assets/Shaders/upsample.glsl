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

layout(location = 0) out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_UpsampleTex;
layout(binding = 1) uniform sampler2D u_BaseTex;

const float upsampleWeight = 0.7;

void main()
{
    vec3 up = texture(u_UpsampleTex, TexCoords).rgb;
    vec3 base = texture(u_BaseTex, TexCoords).rgb;

    FragColor = vec4(mix(base, up, upsampleWeight), 1.0);
}