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

#endif