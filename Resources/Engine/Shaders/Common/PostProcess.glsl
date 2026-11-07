#ifndef POST_PROCESS_GLSL
#define POST_PROCESS_GLSL

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

vec3 AdjustColor(vec3 color, float brightness, float contrast, float saturation)
{
    color += brightness;

    color = (color - 0.5) * contrast + 0.5;

    vec3 luminance = vec3(dot(color, vec3(0.2126, 0.7152, 0.0722)));
    color = mix(luminance, color, saturation);

    return max(color, vec3(0.0));
}

vec3 SampleLUT(sampler2D lutTex, vec3 color)
{
    color = clamp(color, 0.0, 1.0);
    vec2 size = vec2(textureSize(lutTex, 0));
    float lutSize = min(size.x, size.y);
    if (lutSize < 2.0)
        return color;

    float blue = color.b * (lutSize - 1.0);
    float slice0 = floor(blue);
    float slice1 = min(slice0 + 1.0, lutSize - 1.0);
    float fractB = blue - slice0;

    vec2 uv0, uv1;
    if (size.x >= size.y)
    {
        float invWidth = 1.0 / size.x;
        float invHeight = 1.0 / size.y;

        uv0.x = (slice0 * lutSize + 0.5 + color.r * (lutSize - 1.0)) * invWidth;
        uv0.y = (0.5 + color.g * (lutSize - 1.0)) * invHeight;

        uv1.x = (slice1 * lutSize + 0.5 + color.r * (lutSize - 1.0)) * invWidth;
        uv1.y = uv0.y;
    }
    else
    {
        float invWidth = 1.0 / size.x;
        float invHeight = 1.0 / size.y;

        uv0.x = (0.5 + color.r * (lutSize - 1.0)) * invWidth;
        uv0.y = (slice0 * lutSize + 0.5 + color.g * (lutSize - 1.0)) * invHeight;

        uv1.x = uv0.x;
        uv1.y = (slice1 * lutSize + 0.5 + color.g * (lutSize - 1.0)) * invHeight;
    }

    vec3 col0 = textureLod(lutTex, uv0, 0.0).rgb;
    vec3 col1 = textureLod(lutTex, uv1, 0.0).rgb;

    return mix(col0, col1, fractB);
}

#endif
