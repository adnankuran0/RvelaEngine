#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

vec3 BoxSample4Tap(sampler2D tex, vec2 uv, vec2 texelSize)
{
    vec2 uv0 = clamp(uv + vec2(-texelSize.x, -texelSize.y), vec2(0.0), vec2(1.0));
    vec2 uv1 = clamp(uv + vec2( texelSize.x, -texelSize.y), vec2(0.0), vec2(1.0));
    vec2 uv2 = clamp(uv + vec2(-texelSize.x,  texelSize.y), vec2(0.0), vec2(1.0));
    vec2 uv3 = clamp(uv + vec2( texelSize.x,  texelSize.y), vec2(0.0), vec2(1.0));

    vec3 c0 = texture(tex, uv0).rgb;
    vec3 c1 = texture(tex, uv1).rgb;
    vec3 c2 = texture(tex, uv2).rgb;
    vec3 c3 = texture(tex, uv3).rgb;

    return (c0 + c1 + c2 + c3) * 0.25;
}

#endif
