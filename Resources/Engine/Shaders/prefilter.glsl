#shader vertex
#version 460 core
layout(location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 projection;
uniform mat4 view;

void main() {
    localPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core
out vec4 FragColor;
in vec3 localPos;

#include "Common/IBLMath.glsl"

uniform samplerCube environmentMap;
uniform float roughness;

void main() {
    vec3 N = normalize(localPos);
    vec3 V = N;
    
    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    
    for(uint i = 0u; i < SAMPLE_COUNT; i++) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        
        float NdotL = max(dot(N, L), 0.0);
        
        if(NdotL > 0.0) {
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            
            float a = roughness * roughness;
            float a2 = a * a;
            float D = a2 / (PI * pow(NdotH * NdotH * (a2 - 1.0) + 1.0, 2.0));
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;
            
            float resolution = 512.0;
            float saTexel = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
            
            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
            
            vec3 sampleColor = textureLod(environmentMap, L, mipLevel).rgb;
            
            prefilteredColor += sampleColor * NdotL;
            totalWeight += NdotL;
        }
    }
    
    if (totalWeight > 0.0) {
        prefilteredColor = prefilteredColor / totalWeight;
    }
    
    FragColor = vec4(prefilteredColor, 1.0);
}