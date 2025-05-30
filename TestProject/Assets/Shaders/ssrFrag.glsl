#version 410 core

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;

uniform sampler2D uScreenTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uRoughnessTexture;
uniform sampler2D uMetallicTexture;

uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 uInverseProjectionMatrix;
uniform mat4 uInverseViewMatrix;

uniform vec3 uCameraPos;
uniform float near;
uniform float far;

// SSR parameters
const int MAX_STEPS = 64;
const int BINARY_SEARCH_STEPS = 8;
const float INITIAL_RAY_STEP = 0.1;
const float DEPTH_BIAS = 0.05;
const float MAX_DISTANCE = 50.0;
const float MIN_RAY_STEP = 0.01;
const float RAY_HIT_THRESHOLD = 0.01;

// View space position from non-linear depth texture
vec3 getViewPosition(vec2 texCoord, float depthSample) {
    // Screen space to NDC
    vec2 ndc = texCoord * 2.0 - 1.0;
    
    // Depth buffer to linear depth
    float z_ndc = depthSample * 2.0 - 1.0;
    float z_eye = (2.0 * near * far) / (far + near - z_ndc * (far - near));
    
    // View space position
    vec4 viewPos = uInverseProjectionMatrix * vec4(ndc, z_ndc, 1.0);
    viewPos.xyz /= viewPos.w;
    
    return viewPos.xyz;
}

// World space position
vec3 getWorldPosition(vec3 viewPos) {
    vec4 worldPos = uInverseViewMatrix * vec4(viewPos, 1.0);
    return worldPos.xyz;
}

// World position to screen space
vec3 projectToScreen(vec3 worldPos) {
    vec4 clipPos = uProjectionMatrix * uViewMatrix * vec4(worldPos, 1.0);
    vec3 ndcPos = clipPos.xyz / clipPos.w;
    return vec3(ndcPos.xy * 0.5 + 0.5, ndcPos.z * 0.5 + 0.5);
}

// Improved ray marching with adaptive step size
vec3 rayMarch(vec3 rayStart, vec3 rayDir, float roughness, out bool hit) {
    hit = false;
    vec3 currentPos = rayStart;
    
    // Adaptive step size based on roughness
    float rayStep = mix(INITIAL_RAY_STEP, INITIAL_RAY_STEP * 2.0, roughness);
    vec3 step = rayDir * rayStep;
    
    for (int i = 0; i < MAX_STEPS; i++) {
        currentPos += step;
        
        // Project to screen space
        vec3 screenPos = projectToScreen(currentPos);
        
        // Screen boundaries check
        if (screenPos.x < 0.0 || screenPos.x > 1.0 || 
            screenPos.y < 0.0 || screenPos.y > 1.0) {
            break;
        }
        
        // Max distance check
        if (distance(currentPos, rayStart) > MAX_DISTANCE) {
            break;
        }
        
        // Read depth at this point
        float sampledDepth = texture(uDepthTexture, screenPos.xy).r;
        
        // Skip background
        if (sampledDepth > 0.999) continue;
        
        vec3 sampledViewPos = getViewPosition(screenPos.xy, sampledDepth);
        vec3 sampledWorldPos = getWorldPosition(sampledViewPos);
        
        // Calculate hit distance
        float rayDepth = distance(uCameraPos, currentPos);
        float surfaceDepth = distance(uCameraPos, sampledWorldPos);
        
        if (rayDepth > surfaceDepth + DEPTH_BIAS) {
            // Hit found, refine with binary search
            vec3 hitPos = currentPos - step;
            vec3 refinedStep = step * 0.5;
            
            for (int j = 0; j < BINARY_SEARCH_STEPS; j++) {
                hitPos += refinedStep;
                vec3 refinedScreenPos = projectToScreen(hitPos);
                
                if (refinedScreenPos.x >= 0.0 && refinedScreenPos.x <= 1.0 && 
                    refinedScreenPos.y >= 0.0 && refinedScreenPos.y <= 1.0) {
                    
                    float refinedSampledDepth = texture(uDepthTexture, refinedScreenPos.xy).r;
                    if (refinedSampledDepth > 0.999) continue;
                    
                    vec3 refinedSampledViewPos = getViewPosition(refinedScreenPos.xy, refinedSampledDepth);
                    vec3 refinedSampledWorldPos = getWorldPosition(refinedSampledViewPos);
                    
                    float refinedRayDepth = distance(uCameraPos, hitPos);
                    float refinedSurfaceDepth = distance(uCameraPos, refinedSampledWorldPos);
                    
                    if (abs(refinedRayDepth - refinedSurfaceDepth) < RAY_HIT_THRESHOLD) {
                        break; // Good enough hit
                    }
                    
                    if (refinedRayDepth > refinedSurfaceDepth + DEPTH_BIAS) {
                        hitPos -= refinedStep;
                    }
                }
                refinedStep *= 0.5;
            }
            
            hit = true;
            return projectToScreen(hitPos);
        }
        
        // Adaptive step size reduction
        rayStep = max(rayStep * 0.95, MIN_RAY_STEP);
        step = rayDir * rayStep;
    }
    
    return vec3(0.0);
}

void main() {
    vec2 texCoord = TexCoord;
    
    // Read G-Buffer data
    vec3 normal = normalize(texture(uNormalTexture, texCoord).xyz);
    float depth = texture(uDepthTexture, texCoord).r;
    float roughness = texture(uRoughnessTexture, texCoord).r;
    float metallic = texture(uMetallicTexture, texCoord).r;
    
    // Skip background
    if (depth > 0.999) {
        FragColor = vec4(0.0);
        return;
    }
    
    // Calculate reflection probability based on material properties
    float reflectionProbability = mix(1.0 - roughness, 1.0, metallic);
    if (reflectionProbability < 0.1) {
        FragColor = vec4(0.0);
        return;
    }
    
    // View space position
    vec3 viewPos = getViewPosition(texCoord, depth);
    vec3 worldPos = getWorldPosition(viewPos);
    
    // View direction and reflection direction
    vec3 viewDir = normalize(worldPos - uCameraPos);
    vec3 worldNormal = mat3(uInverseViewMatrix) * normal;
    vec3 reflectionDir = reflect(viewDir, normalize(worldNormal));
    
    // Jitter reflection direction based on roughness
    float roughnessJitter = roughness * roughness * 0.2;
    reflectionDir = normalize(reflectionDir + roughnessJitter * vec3(
        fract(sin(dot(texCoord, vec2(12.9898, 78.233))) * 43758.5453) * 2.0 - 1.0,
        fract(sin(dot(texCoord, vec2(39.346, 11.135))) * 87653.2315) * 2.0 - 1.0,
        0.0
    ));
    
    // Ray marching
    bool hit;
    vec3 hitScreenPos = rayMarch(worldPos, reflectionDir, roughness, hit);
    
    if (hit) {
        // Sample reflection color
        vec3 reflectionColor = texture(uScreenTexture, hitScreenPos.xy).rgb;
        
        // Fresnel effect
        float fresnel = pow(1.0 - max(dot(-viewDir, worldNormal), 0.0), 5.0);
        fresnel = mix(mix(0.04, 0.2, fresnel), mix(0.7, 1.0, fresnel), metallic);
        
        // Roughness attenuation (more physically accurate)
        float roughnessAttenuation = 1.0 / (1.0 + roughness * roughness * 10.0);
        
        // Metallic factor
        float metallicFactor = mix(0.3, 1.0, metallic);
        
        // Final reflection strength
        float reflectionStrength = fresnel * roughnessAttenuation * metallicFactor;
        
        // Edge fade
        vec2 edgeFade = smoothstep(0.0, 0.1, hitScreenPos.xy) * 
                       smoothstep(1.0, 0.9, hitScreenPos.xy);
        reflectionStrength *= edgeFade.x * edgeFade.y;
        
        // Apply screen-space blur based on roughness
        if (roughness > 0.3) {
            float blurRadius = roughness * 5.0;
            vec3 blurredReflection = vec3(0.0);
            float totalWeight = 0.0;
            
            for (int x = -2; x <= 2; x++) {
                for (int y = -2; y <= 2; y++) {
                    vec2 offset = vec2(x, y) * (blurRadius / 512.0);
                    float weight = 1.0 / (1.0 + length(vec2(x, y)));
                    blurredReflection += texture(uScreenTexture, hitScreenPos.xy + offset).rgb * weight;
                    totalWeight += weight;
                }
            }
            
            reflectionColor = blurredReflection / totalWeight;
        }
        
        FragColor = vec4(reflectionColor, reflectionStrength);
    } else {
        FragColor = vec4(0.0);
    }
}