#ifndef SPHERICAL_MATH_GLSL
#define SPHERICAL_MATH_GLSL

#include "Constants.glsl"

const vec2 invAtan = vec2(0.15915494309, 0.31830988618); // vec2(0.5 / PI, 1.0 / PI)

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(clamp(v.y, -1.0, 1.0)));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

#endif