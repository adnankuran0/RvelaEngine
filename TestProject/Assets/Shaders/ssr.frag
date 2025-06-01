#version 460 core

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D uDepthTexture;
layout(binding = 1) uniform sampler2D uNormalTexture;
layout(binding = 2) uniform sampler2D uMetallicTexture;
layout(binding = 3) uniform sampler2D uRoughnessTexture;
layout(binding = 4) uniform sampler2D uScreenTexture;

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

// Precomputed values
const float linearDepthConst = 2.0 * near * far;
const float depthDenomConst = far - near;

vec3 getViewPosition(vec2 texCoord, float depthSample) {
    vec2 ndc = texCoord * 2.0 - 1.0;
    float z_ndc = depthSample * 2.0 - 1.0;
    
    float z_eye = linearDepthConst / (far + near - z_ndc * depthDenomConst);
    
    vec4 viewPos = uInverseProjectionMatrix * vec4(ndc, z_ndc, 1.0);
    return viewPos.xyz / viewPos.w;
}

vec3 projectToScreen(vec3 worldPos) {
    vec4 clipPos = uProjectionMatrix * (uViewMatrix * vec4(worldPos, 1.0));
    vec3 ndcPos = clipPos.xyz / clipPos.w;
    return vec3(ndcPos.xy * 0.5 + 0.5, ndcPos.z * 0.5 + 0.5);
}

vec3 rayMarch(vec3 rayStart, vec3 rayDir, float roughness, out bool hit) 
{
    hit = false;
    vec3 currentPos = rayStart;
    
    // Precomputed adaptive step factors
    float rayStep = mix(INITIAL_RAY_STEP, INITIAL_RAY_STEP * 2.0, roughness);
    float stepFactor = 0.95;
    vec3 stepVec = rayDir * rayStep;
    float maxDistSq = MAX_DISTANCE * MAX_DISTANCE;
    float distSq = 0.0;
    
    for (int i = 0; i < MAX_STEPS; i++) 
    {
        currentPos += stepVec;
        vec3 screenPos = projectToScreen(currentPos);
        
        // Early exit checks
        if (any(lessThan(screenPos.xy, vec2(0.0)))) break;
        if (any(greaterThan(screenPos.xy, vec2(1.0)))) break;
        
        vec3 toCurrent = currentPos - uCameraPos;
        distSq = dot(toCurrent, toCurrent);
        if (distSq > maxDistSq) break;
        
        float sampledDepth = texture(uDepthTexture, screenPos.xy).r;
        if (sampledDepth > 0.999) continue;
        
        float rayDepth = sqrt(distSq);
        vec3 sampledViewPos = getViewPosition(screenPos.xy, sampledDepth);
        vec4 sampledWorldPosTemp = uInverseViewMatrix * vec4(sampledViewPos, 1.0);
        vec3 sampledWorldPos = sampledWorldPosTemp.xyz / sampledWorldPosTemp.w;
        
        float surfaceDepth = distance(uCameraPos, sampledWorldPos);
        if (rayDepth > surfaceDepth + DEPTH_BIAS) 
        {
            vec3 hitPos = currentPos - stepVec;
            vec3 refineStep = stepVec * 0.5;
            
            for (int j = 0; j < BINARY_SEARCH_STEPS; j++) {
                hitPos += refineStep;
                vec3 refinedScreenPos = projectToScreen(hitPos);
                
                if (all(greaterThanEqual(refinedScreenPos.xy, vec2(0.0))) && 
                    all(lessThanEqual(refinedScreenPos.xy, vec2(1.0)))) {
                    
                    float refinedDepth = texture(uDepthTexture, refinedScreenPos.xy).r;
                    if (refinedDepth < 0.999) {
                        vec3 refinedSampledViewPos = getViewPosition(refinedScreenPos.xy, refinedDepth);
                        vec4 refinedSampledWorldPosTemp = uInverseViewMatrix * vec4(refinedSampledViewPos, 1.0);
                        vec3 refinedSampledWorldPos = refinedSampledWorldPosTemp.xyz / refinedSampledWorldPosTemp.w;
                        
                        float refinedRayDepth = distance(uCameraPos, hitPos);
                        float refinedSurfaceDepth = distance(uCameraPos, refinedSampledWorldPos);
                        
                        if (abs(refinedRayDepth - refinedSurfaceDepth) < RAY_HIT_THRESHOLD) break;
                        
                        if (refinedRayDepth > refinedSurfaceDepth + DEPTH_BIAS) {
                            hitPos -= refineStep;
                        }
                    }
                }
                refineStep *= 0.5;
            }
        
            
            hit = true;
            return projectToScreen(hitPos);
        }
        
        rayStep = max(rayStep * stepFactor, MIN_RAY_STEP);
        stepVec = rayDir * rayStep;
    }
    
    return vec3(0.0);
}

void main() {
    vec2 texCoord = TexCoord;
    
    float depth = texture(uDepthTexture, texCoord).r;
    if (depth > 0.999) {
        FragColor = vec4(0.0);
        return;
    }
    
    vec3 normal = texture(uNormalTexture, texCoord).xyz;
    float roughness = texture(uRoughnessTexture, texCoord).r;
    float metallic = texture(uMetallicTexture, texCoord).r;
    
    // Early exit for non-reflective surfaces
    float reflectionProbability = mix(1.0 - roughness, 1.0, metallic);
    if (reflectionProbability < 0.1) {
        FragColor = vec4(0.0);
        return;
    }
    
    vec3 viewPos = getViewPosition(texCoord, depth);
    vec4 worldPosTemp = uInverseViewMatrix * vec4(viewPos, 1.0);
    vec3 worldPos = worldPosTemp.xyz / worldPosTemp.w;
    
    vec3 viewDir = normalize(worldPos - uCameraPos);
    vec3 worldNormal = mat3(uInverseViewMatrix) * normal;
    vec3 reflectionDir = reflect(viewDir, normalize(worldNormal));
    
    float roughnessJitter = roughness * roughness * 0.2;
    float rand1 = fract(sin(dot(texCoord, vec2(12.9898, 78.233))) * 43758.5453);
    float rand2 = fract(sin(dot(texCoord, vec2(39.346, 11.135))) * 87653.2315);
    vec3 jitter = vec3(rand1 * 2.0 - 1.0, rand2 * 2.0 - 1.0, 0.0);
    reflectionDir = normalize(reflectionDir + roughnessJitter * jitter);
    
    bool hit;
    vec3 hitScreenPos = rayMarch(worldPos, reflectionDir, roughness, hit);
    
    if (hit) {
        vec2 edgeFade = smoothstep(0.0, 0.1, hitScreenPos.xy) * 
                       smoothstep(1.0, 0.9, hitScreenPos.xy);
        float fadeFactor = edgeFade.x * edgeFade.y;
        
        if (fadeFactor < 0.01) {
            FragColor = vec4(0.0);
            return;
        }
        
        vec3 reflectionColor = texture(uScreenTexture, hitScreenPos.xy).rgb;
        
        float fresnel = pow(1.0 - max(dot(-viewDir, worldNormal), 0.0), 5.0);
        float fresnelFactor = mix(mix(0.04, 0.2, fresnel), mix(0.7, 1.0, fresnel), metallic);
        
        // Material attenuation
        float roughnessAtt = 1.0 / (1.0 + roughness * roughness * 10.0);
        float reflectionStrength = fresnelFactor * roughnessAtt * mix(0.3, 1.0, metallic) * fadeFactor;
        
        // Blur for rough surfaces
        if (roughness > 0.1) {
            float blurRadius = roughness * 5.0;
            vec2 texelSize = 1.0 / textureSize(uScreenTexture, 0);
            vec3 blurred = vec3(0.0);
            float totalWeight = 0.0;
            
            for (int x = -2; x <= 2; ++x) {
                for (int y = -2; y <= 2; ++y) {
                    vec2 offset = vec2(x, y) * texelSize * blurRadius;
                    float weight = 1.0 / (1.0 + abs(x) + abs(y)); // Manhattan distance
                    blurred += texture(uScreenTexture, hitScreenPos.xy + offset).rgb * weight;
                    totalWeight += weight;
                }
            }
            reflectionColor = blurred / totalWeight;
        }
        
        FragColor = vec4(reflectionColor, reflectionStrength);
    } else {
        FragColor = vec4(0.0);
    }
}