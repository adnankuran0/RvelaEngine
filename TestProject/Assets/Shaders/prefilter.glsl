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

uniform samplerCube environmentMap;
uniform float roughness;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangentX = normalize(cross(up, N));
    vec3 tangentY = cross(N, tangentX);
    
    return normalize(tangentX * H.x + tangentY * H.y + N * H.z);
}

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