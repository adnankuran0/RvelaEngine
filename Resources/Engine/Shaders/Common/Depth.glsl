#ifndef DEPTH_GLSL
#define DEPTH_GLSL

vec3 ReconstructViewPos(vec2 uv, float depth, mat4 invProj)
{
    vec4 ndc = vec4(
        uv.x * 2.0 - 1.0,
        uv.y * 2.0 - 1.0,
        depth * 2.0 - 1.0,
        1.0
    );
    vec4 viewPos = invProj * ndc;
    return viewPos.xyz / viewPos.w;
}

#endif