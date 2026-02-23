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

uniform float chromaticStrength;   // 0.0 – 1.0

uniform float vignetteIntensity;   // 0.0 – 1.0
uniform float vignetteSmoothness;  // 0.0 – 1.0

vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}


vec3 ChromaticAberration(sampler2D tex, vec2 uv, float strength)
{
    vec2 direction = uv - 0.5;
    direction *= direction;
    float redCoeff   =  strength;
    float greenCoeff =  0.0;
    float blueCoeff  = -strength;

    vec2 uvRed   = clamp(uv + direction * redCoeff,   0.0, 1.0);
    vec2 uvGreen = clamp(uv + direction * greenCoeff, 0.0, 1.0);
    vec2 uvBlue  = clamp(uv + direction * blueCoeff,  0.0, 1.0);

    float r = texture(tex, uvRed).r;
    float g = texture(tex, uvGreen).g;
    float b = texture(tex, uvBlue).b;

    return vec3(r, g, b);
}

float Vignette(vec2 uv, float intensity, float smoothness)
{
    vec2 d = uv - vec2(0.5);
    float dist = length(d);

   
    float innerRadius = mix(0.55, 0.10, smoothness);
    float outerRadius = mix(0.60, 0.90, smoothness);

    float vignette = smoothstep(innerRadius, outerRadius, dist);
    return 1.0 - vignette * intensity;
}

void main()
{
    vec3 hdrColor   = ChromaticAberration(screenTexture, TexCoords, chromaticStrength);
    vec3 bloomColor = texture(bloomTexture, TexCoords).rgb;
    vec4 ssrSample  = texture(ssrTexture,  TexCoords);

    float blurredAO = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            blurredAO += texture(aoTexture, TexCoords + vec2(x, y) / textureSize(aoTexture, 0)).r;
    float ao = blurredAO / 9.0;

    vec3 ambient = hdrColor * 0.1;
    hdrColor -= ambient * (1.0 - ao) * 10.0;
    hdrColor  = max(hdrColor, vec3(0.0));

    hdrColor = mix(hdrColor, hdrColor + ssrSample.rgb, ssrSample.a);

    vec3 combined = hdrColor + bloomColor;

    combined    = vec3(1.0) - exp(-combined * exposure);
    vec3 mapped = ACESFilm(combined);
    
    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));

    mapped *= Vignette(TexCoords, vignetteIntensity, vignetteSmoothness);

    FragColor = vec4(mapped, 1.0);
}