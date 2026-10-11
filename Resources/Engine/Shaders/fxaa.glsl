#shader vertex
#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

#shader fragment
#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_Texture;
uniform vec2 u_TexelSize;
uniform bool u_FXAAEnabled;

#define FXAA_SPAN_MAX 8.0
#define FXAA_REDUCE_MUL   (1.0 / 8.0)
#define FXAA_REDUCE_MIN   (1.0 / 128.0)
#define FXAA_SUBPIX_SHIFT (1.0 / 4.0)

float FxaaLuma(vec3 rgb)
{
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec3 rgbM = texture(u_Texture, TexCoords).rgb;
    if (!u_FXAAEnabled)
    {
        FragColor = vec4(rgbM, 1.0);
        return;
    }

    vec3 rgbNW = texture(u_Texture, TexCoords + vec2(-1.0, -1.0) * u_TexelSize).rgb;
    vec3 rgbNE = texture(u_Texture, TexCoords + vec2( 1.0, -1.0) * u_TexelSize).rgb;
    vec3 rgbSW = texture(u_Texture, TexCoords + vec2(-1.0,  1.0) * u_TexelSize).rgb;
    vec3 rgbSE = texture(u_Texture, TexCoords + vec2( 1.0,  1.0) * u_TexelSize).rgb;

    float lumaNW = FxaaLuma(rgbNW);
    float lumaNE = FxaaLuma(rgbNE);
    float lumaSW = FxaaLuma(rgbSW);
    float lumaSE = FxaaLuma(rgbSE);
    float lumaM  = FxaaLuma(rgbM);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    float lumaRange = lumaMax - lumaMin;
    if (lumaRange < max(0.05, lumaMax * 0.125))
    {
        FragColor = vec4(rgbM, 1.0);
        return;
    }

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX), dir * rcpDirMin)) * u_TexelSize;

    vec3 rgbA = 0.5 * (
        texture(u_Texture, TexCoords + dir * (1.0 / 3.0 - 0.5)).rgb +
        texture(u_Texture, TexCoords + dir * (2.0 / 3.0 - 0.5)).rgb);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(u_Texture, TexCoords + dir * (0.0 / 3.0 - 0.5)).rgb +
        texture(u_Texture, TexCoords + dir * (3.0 / 3.0 - 0.5)).rgb);

    float lumaB = FxaaLuma(rgbB);

    if ((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        FragColor = vec4(rgbA, 1.0);
    }
    else
    {
        FragColor = vec4(rgbB, 1.0);
    }
}
