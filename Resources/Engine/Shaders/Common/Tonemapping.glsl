#ifndef TONEMAPPING_GLSL
#define TONEMAPPING_GLSL

vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 LinearToSRGB(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

vec3 SRGBToLinear(vec3 color)
{
    return pow(color, vec3(2.2));
}

#endif