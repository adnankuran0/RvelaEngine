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

layout(binding = 0) uniform sampler2D gNormal;
layout(binding = 1) uniform sampler2D gDepth;
layout(binding = 2) uniform sampler2D texNoise;
uniform mat4 projection;
uniform mat4 invProjection; 
uniform vec2 windowSize;
uniform vec3 samples[32];

const float radius = 1.0;      
const float bias = 0.025;
const float intensity = 1.0;
const float invSamples = 1.0 / 32.0;

uniform float near;
uniform float far;

vec3 getViewPos(vec2 uv)
{
    float depthValue = texture(gDepth, uv).r;
    vec4 ndc = vec4(
        uv.s * 2.0 - 1.0,
        uv.t * 2.0 - 1.0,
        depthValue * 2.0 - 1.0,
        1.0
    );
    vec4 viewPos = invProjection * ndc;
    return viewPos.xyz / viewPos.w;
}

void main()
{
    vec3 fragPos = getViewPos(TexCoords);
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec2 noiseScale = windowSize * 0.25;
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

        float sampleDepth = getViewPos(sampleUV).z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));

        occlusion += (sampleDepth >= samplePos.z + bias) ? rangeCheck : 0.0;
    }
    
    FragColor = pow(1.0 - (occlusion * invSamples), intensity);
}