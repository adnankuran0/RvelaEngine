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
out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_SSRTexture;

void main()
{
    vec4 ssr = texture(u_SSRTexture, TexCoords);
    if (ssr.a <= 0.001)
    {
        discard;
    }
    FragColor = vec4(ssr.rgb, ssr.a);
}
