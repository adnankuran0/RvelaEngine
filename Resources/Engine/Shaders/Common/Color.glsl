#ifndef COLOR_GLSL
#define COLOR_GLSL

float Luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

#endif