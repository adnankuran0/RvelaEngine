#ifndef FULLSCREEN_QUAD_GLSL
#define FULLSCREEN_QUAD_GLSL

void OutputFullscreenQuad(vec2 inPos, vec2 inTexCoords, out vec2 outTexCoords)
{
    outTexCoords = inTexCoords;
    gl_Position = vec4(inPos.xy, 0.0, 1.0);
}

#endif