#shader vertex
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 3) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 UVScale;
uniform vec2 UVOffset;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords * UVScale + UVOffset;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core
in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

uniform vec4 u_Color;
layout(binding = 0) uniform sampler2D albedoMap;
uniform bool useAlbedoMap;
uniform vec4 albedoColor;
uniform int transparencyMode;
uniform float alphaCutoff;

void main()
{
    if (transparencyMode == 2) // alpha scissor
    {
        float alpha = (useAlbedoMap ? texture(albedoMap, TexCoords).a : 1.0) * albedoColor.a;
        if (alpha < alphaCutoff)
        {
            discard;
        }
    }

    FragColor = u_Color;
}