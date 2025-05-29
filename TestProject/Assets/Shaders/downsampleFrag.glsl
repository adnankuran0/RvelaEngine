#version 400 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_Texture;
uniform vec2 u_TexelSize;

void main()
{
    vec3 result = texture(u_Texture, TexCoords).rgb * 0.25;
    result += texture(u_Texture, TexCoords + vec2(u_TexelSize.x, 0.0)).rgb * 0.25;
    result += texture(u_Texture, TexCoords + vec2(0.0, u_TexelSize.y)).rgb * 0.25;
    result += texture(u_Texture, TexCoords + u_TexelSize).rgb * 0.25;

    FragColor = vec4(result, 1.0);
}