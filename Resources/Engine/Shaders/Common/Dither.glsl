#ifndef RV_DITHER_GLSL
#define RV_DITHER_GLSL

float GetDitherThreshold(vec2 screenPos)
{
    const vec3 c = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(c.z * fract(dot(screenPos, c.xy)));
}

#endif
