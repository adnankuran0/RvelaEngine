#version 400 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_UpsampleTex;
uniform sampler2D u_BaseTex;
uniform vec2 u_TexelSize;

void main()
{
    vec3 upsampled = texture(u_UpsampleTex, TexCoords).rgb;
    vec3 base = texture(u_BaseTex, TexCoords).rgb;
    float upsampleWeight = 0.7;
    float baseWeight = 0.3;

    FragColor = vec4(upsampled * upsampleWeight + base * baseWeight, 1.0);
}
