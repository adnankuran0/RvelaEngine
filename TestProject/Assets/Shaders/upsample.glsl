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
layout(binding = 1) uniform sampler2D u_BaseMip;

uniform float u_Intensity = 1.0;
uniform float u_Radius = 1.0;

void main()
{
    vec3 base = texture(u_BaseMip, TexCoords).rgb;

    vec2 texelSize = 1.0 / vec2(textureSize(u_LowMip, 0));

    vec3 up = vec3(0.0);
    up += texture(u_LowMip, TexCoords).rgb * 4.0;
    up += texture(u_LowMip, TexCoords + vec2(texelSize.x, 0.0) * u_Radius).rgb;
    up += texture(u_LowMip, TexCoords - vec2(texelSize.x, 0.0) * u_Radius).rgb;
    up += texture(u_LowMip, TexCoords + vec2(0.0, texelSize.y) * u_Radius).rgb;
    up += texture(u_LowMip, TexCoords - vec2(0.0, texelSize.y) * u_Radius).rgb;

    up /= 8.0;

    vec3 result = base + up * u_Intensity;

    FragColor = vec4(result, 1.0);
}