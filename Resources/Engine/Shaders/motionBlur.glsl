#shader vertex
#version 460 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment
#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_ScreenTexture;
layout(binding = 1) uniform sampler2D u_DepthTexture;

uniform mat4 u_InvViewProj;
uniform mat4 u_PrevViewProj;
uniform float u_Intensity;
uniform int u_Samples;
uniform float u_MaxVelocity;

float InterleavedGradientNoise(vec2 screenPos)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

void main()
{
    float depth = texture(u_DepthTexture, TexCoords).r;

    vec4 clipPos = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = u_InvViewProj * clipPos;
    worldPos /= worldPos.w;

    vec4 prevClipPos = u_PrevViewProj * worldPos;

    if (prevClipPos.w <= 0.001)
    {
        FragColor = texture(u_ScreenTexture, TexCoords);
        return;
    }

    vec2 prevNDC = prevClipPos.xy / prevClipPos.w;
    vec2 prevTexCoords = prevNDC * 0.5 + 0.5;

    vec2 velocity = (TexCoords - prevTexCoords) * u_Intensity;

    bool isSky = (depth >= 0.99999);
    if (isSky)
    {
        velocity *= 0.5;
    }

    float speed = length(velocity);

    if (speed < 0.0002)
    {
        FragColor = texture(u_ScreenTexture, TexCoords);
        return;
    }

    float maxVel = max(u_MaxVelocity, 0.001);
    float compressedSpeed = maxVel * (1.0 - exp(-speed / maxVel));
    velocity = (velocity / speed) * compressedSpeed;

    float jitter = InterleavedGradientNoise(gl_FragCoord.xy);
    int sampleCount = clamp(u_Samples, 4, 16);

    vec4 totalColor = vec4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < sampleCount; ++i)
    {
        float t = (float(i) + jitter) / float(sampleCount) - 0.5;
        vec2 sampleUV = TexCoords + velocity * t;
        sampleUV = clamp(sampleUV, vec2(0.001), vec2(0.999));

        float distFromCenter = abs(t) * 2.0;
        float weight = 1.0 - 0.5 * distFromCenter;

        if (!isSky)
        {
            float sampleDepth = texture(u_DepthTexture, sampleUV).r;
            if (sampleDepth < depth * 0.85)
            {
                weight *= 0.2;
            }
        }

        totalColor += texture(u_ScreenTexture, sampleUV) * weight;
        totalWeight += weight;
    }

    FragColor = totalColor / max(totalWeight, 0.0001);
}
