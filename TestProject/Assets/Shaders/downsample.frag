#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_Texture;
uniform vec2 u_TexelSize;

void main()
{
    vec2 texCoord00 = TexCoords + vec2(-u_TexelSize.x,  u_TexelSize.y);
    vec2 texCoord10 = TexCoords + vec2(0.0,             u_TexelSize.y);
    vec2 texCoord20 = TexCoords + vec2( u_TexelSize.x,  u_TexelSize.y);
    
    vec2 texCoord01 = TexCoords + vec2(-u_TexelSize.x,  0.0);
    vec2 texCoord11 = TexCoords; // Center coordinate
    vec2 texCoord21 = TexCoords + vec2( u_TexelSize.x,  0.0);
    
    vec2 texCoord02 = TexCoords + vec2(-u_TexelSize.x, -u_TexelSize.y);
    vec2 texCoord12 = TexCoords + vec2(0.0,            -u_TexelSize.y);
    vec2 texCoord22 = TexCoords + vec2( u_TexelSize.x, -u_TexelSize.y);

    vec3 color00 = texture(u_Texture, texCoord00).rgb;
    vec3 color10 = texture(u_Texture, texCoord10).rgb;
    vec3 color20 = texture(u_Texture, texCoord20).rgb;
    
    vec3 color01 = texture(u_Texture, texCoord01).rgb;
    vec3 color11 = texture(u_Texture, texCoord11).rgb;
    vec3 color21 = texture(u_Texture, texCoord21).rgb;
    
    vec3 color02 = texture(u_Texture, texCoord02).rgb;
    vec3 color12 = texture(u_Texture, texCoord12).rgb;
    vec3 color22 = texture(u_Texture, texCoord22).rgb;

    vec3 color = 
        color00 * 1.0 + color10 * 2.0 + color20 * 1.0 +
        color01 * 2.0 + color11 * 4.0 + color21 * 2.0 +
        color02 * 1.0 + color12 * 2.0 + color22 * 1.0;

    FragColor = vec4(color / 16.0, 1.0);
}