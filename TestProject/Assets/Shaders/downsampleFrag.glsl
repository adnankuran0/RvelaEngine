#version 400 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec2 u_TexelSize;

void main()
{
    vec2 offsets[9] = vec2[](
        vec2(-u_TexelSize.x,  u_TexelSize.y),
        vec2(0.0,             u_TexelSize.y),
        vec2( u_TexelSize.x,  u_TexelSize.y),

        vec2(-u_TexelSize.x,  0.0),
        vec2(0.0,             0.0),
        vec2( u_TexelSize.x,  0.0),

        vec2(-u_TexelSize.x, -u_TexelSize.y),
        vec2(0.0,            -u_TexelSize.y),
        vec2( u_TexelSize.x, -u_TexelSize.y)
    );

    float weights[9] = float[](
        1.0, 2.0, 1.0,
        2.0, 4.0, 2.0,
        1.0, 2.0, 1.0
    );

    vec3 color = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 9; ++i)
    {
        vec3 texelColor = texture(u_Texture, TexCoords + offsets[i]).rgb;
        color += texelColor * weights[i];
        totalWeight += weights[i];
    }

    color /= totalWeight;
    FragColor = vec4(color, 1.0);
}