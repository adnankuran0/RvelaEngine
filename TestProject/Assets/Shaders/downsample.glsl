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

layout(binding = 0) uniform sampler2D u_Texture;
uniform vec2 u_TexelSize;

void main()
{
    vec2 ts = u_TexelSize;

    vec3 c0 = texture(u_Texture, TexCoords + vec2(-ts.x, -ts.y)).rgb;
    vec3 c1 = texture(u_Texture, TexCoords + vec2( ts.x, -ts.y)).rgb;
    vec3 c2 = texture(u_Texture, TexCoords + vec2(-ts.x,  ts.y)).rgb;
    vec3 c3 = texture(u_Texture, TexCoords + vec2( ts.x,  ts.y)).rgb;

    FragColor = vec4((c0 + c1 + c2 + c3) * 0.25, 1.0);
}
