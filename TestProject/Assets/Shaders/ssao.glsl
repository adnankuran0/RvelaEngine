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
out float FragColor;
in vec2 TexCoords;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"

layout(binding = 0) uniform sampler2D gNormal;
layout(binding = 1) uniform sampler2D gDepth;
layout(binding = 2) uniform sampler2D texNoise;

uniform vec3 samples[32];
const float invSamples = 1.0 / 32.0;

uniform float radius = 1.0;      
uniform float bias = 0.025;
uniform float intensity = 1.0;

void main()
{
    float depth = texture(gDepth, TexCoords).r;
    vec3 fragPos = ReconstructViewPos(TexCoords, depth, invProjection);
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);

    vec2 noiseScale = (windowSize * 0.5) * 0.25;
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    float occlusion = 0.0;
    for(int i = 0; i < 32; ++i)
    {
        vec3 samplePos = fragPos + (TBN * samples[i]) * radius;
        
        vec4 clipPos = projection * vec4(samplePos, 1.0);
        vec3 projCoords = clipPos.xyz / clipPos.w;
        vec2 sampleUV = projCoords.xy * 0.5 + 0.5;

        float sampleDepthVal = texture(gDepth, sampleUV).r;
        float sampleDepth = ReconstructViewPos(sampleUV, sampleDepthVal, invProjection).z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));

        occlusion += (sampleDepth >= samplePos.z + bias) ? rangeCheck : 0.0;
    }
    
    FragColor = pow(1.0 - (occlusion * invSamples), intensity);
}