#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

vec3 BoxSample4Tap(sampler2D tex, vec2 uv, vec2 texelSize)
{
    vec3 c0 = texture(tex, uv + vec2(-texelSize.x, -texelSize.y)).rgb;
    vec3 c1 = texture(tex, uv + vec2( texelSize.x, -texelSize.y)).rgb;
    vec3 c2 = texture(tex, uv + vec2(-texelSize.x,  texelSize.y)).rgb;
    vec3 c3 = texture(tex, uv + vec2( texelSize.x,  texelSize.y)).rgb;

    return (c0 + c1 + c2 + c3) * 0.25;
}

#endif