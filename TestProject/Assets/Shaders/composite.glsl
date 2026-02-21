#shader vertex
#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

#shader fragment
#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D screenTexture;
layout(binding = 1) uniform sampler2D bloomTexture;
layout(binding = 2) uniform sampler2D aoTexture;
layout(binding = 3) uniform sampler2D ssrTexture;
uniform float exposure;

vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
    vec3 bloomColor = texture(bloomTexture, TexCoords).rgb;
    vec4 ssrSample = texture(ssrTexture, TexCoords);
    
    float blurredAO = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            blurredAO += texture(aoTexture, TexCoords + vec2(x, y) / textureSize(aoTexture, 0)).r;
        }
    }
    float ao = blurredAO / 9.0;
    
    vec3 ambient = hdrColor * 0.03;
    hdrColor -= ambient * (1.0 - ao) * 2.0;
    hdrColor = max(hdrColor, vec3(0.0));
    
    hdrColor = mix(hdrColor, hdrColor + ssrSample.rgb, ssrSample.a);
    
    vec3 combined = hdrColor + bloomColor;

    combined = vec3(1.0) - exp(-combined * exposure);
    vec3 mapped = ACESFilm(combined);

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));
    FragColor = vec4(mapped, 1.0);
}