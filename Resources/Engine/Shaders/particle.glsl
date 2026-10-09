#shader vertex
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 3) in vec2 aTexCoords; 

#include "Common/Camera.glsl"

struct ParticleInstanceData
{
    vec4 positionAndScale;
    vec4 color;
    vec4 rotationAndCustom;
};

layout(std430, binding = 0) readonly buffer InstanceBuffer {
    ParticleInstanceData instances[];
};

uniform int instanceOffset;
uniform int billboardMode;

uniform vec2 UVScale;
uniform vec2 UVOffset;

out vec2 TexCoords;
out vec4 ParticleColor;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    ParticleInstanceData data = instances[gl_InstanceID + instanceOffset];
    
    vec3 instPos = data.positionAndScale.xyz;
    float scale = data.positionAndScale.w;
    float rot = data.rotationAndCustom.x;
    
    ParticleColor = data.color;
    TexCoords = (aTexCoords * UVScale) + UVOffset;

    vec3 vertexPos;
    vec3 N = aNormal;

    if (billboardMode == 1) // Spherical
    {
        vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);
        
        float cosRot = cos(rot);
        float sinRot = sin(rot);
        vec3 right = camRight * cosRot - camUp * sinRot;
        vec3 up    = camRight * sinRot + camUp * cosRot;
        
        vertexPos = instPos + (right * aPos.x + up * aPos.y + camRight * aPos.z) * scale;
        N = normalize(camPos - instPos);
    }
    else if (billboardMode == 2) // Cylindrical
    {
        vec3 look = camPos - instPos;
        look.y = 0.0;
        
        if (length(look) > 0.0001)
            look = normalize(look);
        else
            look = vec3(0.0, 0.0, 1.0);

        vec3 up = vec3(0.0, 1.0, 0.0);
        vec3 right = normalize(cross(up, look));

        float cosRot = cos(rot);
        float sinRot = sin(rot);
        vec3 finalRight = right * cosRot - up * sinRot;
        vec3 finalUp    = right * sinRot + up * cosRot;

        vertexPos = instPos + (finalRight * aPos.x + finalUp * aPos.y + look * aPos.z) * scale;
        N = look;
    }
    else // Disabled
    {
        float cosRot = cos(rot);
        float sinRot = sin(rot);
        vec3 rotatedPos = vec3(
            aPos.x * cosRot - aPos.y * sinRot,
            aPos.x * sinRot + aPos.y * cosRot,
            aPos.z
        );
        vertexPos = instPos + (rotatedPos * scale);
    }

    FragPos = vertexPos;
    Normal = N;

    gl_Position = projection * view * vec4(vertexPos, 1.0);
}


#shader fragment
#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec4 ParticleColor;
in vec3 FragPos;
in vec3 Normal;

#include "Common/Camera.glsl"
#include "Common/Lights.glsl"
#include "Common/Shadows.glsl"

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 6) uniform sampler2D shadowMap;
layout(binding = 7) uniform samplerCubeArray pointShadowMap;
uniform bool receiveShadows;

uniform bool useAlbedoMap;
uniform vec4 albedoColor;

uniform vec3 emmisiveColor;
uniform float emmisiveIntensity;

uniform int transparencyMode;
uniform float alphaCutoff;

uniform int shadingMode;
uniform vec3 ambientColor;
uniform float ambientIntensity;

float calculateParticleDirShadow(sampler2D sMap, vec4 lightSpacePos, vec3 normal, vec3 lightDir, float shadowBias) {
    vec3 proj = lightSpacePos.xyz / lightSpacePos.w;
    proj = proj * 0.5 + 0.5;

    if (proj.z > 1.0 || any(lessThan(proj.xy, vec2(0.0))) || any(greaterThan(proj.xy, vec2(1.0))))
        return 0.0;

    float bias = min(shadowBias * (1.0 - dot(normal, lightDir)), shadowBias);
    float currentDepth = proj.z - bias;

    vec2 texelSize = 1.0 / textureSize(sMap, 0);
    float s0 = step(texture(sMap, proj.xy + vec2(-0.5, -0.5) * texelSize).r, currentDepth);
    float s1 = step(texture(sMap, proj.xy + vec2( 0.5, -0.5) * texelSize).r, currentDepth);
    float s2 = step(texture(sMap, proj.xy + vec2(-0.5,  0.5) * texelSize).r, currentDepth);
    float s3 = step(texture(sMap, proj.xy + vec2( 0.5,  0.5) * texelSize).r, currentDepth);
    return (s0 + s1 + s2 + s3) * 0.25;
}

float calculateParticlePointShadow(samplerCubeArray pMap, int index, vec3 fragPos, vec3 lightPos, float farPlane, vec3 normal, float shadowBias) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    if (currentDepth > farPlane) return 0.0;

    vec3 L = normalize(lightPos - fragPos);
    float bias = max(shadowBias * (1.0 - dot(normal, L)), shadowBias * 0.05);
    float closestDepth = texture(pMap, vec4(fragToLight, float(index))).r * farPlane;
    return step(closestDepth, currentDepth - bias);
}

void main()
{
    vec4 texColor = useAlbedoMap ? texture(albedoMap, TexCoords) : vec4(1.0);
    vec4 finalColor = texColor * albedoColor * ParticleColor;
    
    float alpha = finalColor.a;

    float camDist = length(FragPos - camPos);
    float nearFade = clamp((camDist - nearPlane) / 0.5, 0.0, 1.0);
    alpha *= nearFade;

    if (transparencyMode == 0) 
    {
        alpha = 1.0;
    } 
    else if (transparencyMode == 2) 
    {
        if (alpha < alphaCutoff) discard;
        alpha = 1.0; 
    }

    if (alpha < 0.01)
        discard;
        
    vec3 albedo = finalColor.rgb;
    vec3 resultRGB = albedo;

    if (shadingMode == 0) 
    {
        vec3 N = normalize(Normal);
        if (!gl_FrontFacing) N = -N;

        vec3 ambient = ambientColor * ambientIntensity * albedo;
        vec3 Lo = vec3(0.0);

        if (hasDirectionalLight == 1) {
            vec3 L = normalize(-directionalLight.direction.xyz);
            
            float NdotL = clamp(dot(N, L) * 0.4 + 0.6, 0.0, 1.0);
            
            bool castShadows = directionalLight.direction.w > 0.5;
            vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
            float shadow = (castShadows && receiveShadows) ?
                calculateParticleDirShadow(shadowMap, fragPosLightSpace, N, L, directionalLight.shadowBias) : 0.0;

            vec3 lightColor = directionalLight.colorIntensity.rgb;
            float lightIntensity = directionalLight.colorIntensity.a;

            Lo += (1.0 - shadow) * albedo * lightColor * lightIntensity * NdotL;
        }

        for (int i = 0; i < pointLightCount; ++i) {
            PointLight light = pointLights[i];
            vec3 L_vec = light.position.xyz - FragPos;
            float dist2 = dot(L_vec, L_vec);
            float radius2 = light.radius * light.radius;
            
            if (dist2 > radius2) continue;
            
            float distance = sqrt(dist2);
            vec3 L = L_vec / distance;
            
            float num = clamp(1.0 - (dist2 / radius2) * (dist2 / radius2), 0.0, 1.0);
            float attenuation = (num * num) / (dist2 + 1.0);
            
            float NdotL = clamp(dot(N, L) * 0.4 + 0.6, 0.0, 1.0);
            
            bool castShadows = light.shadowIndex >= 0;
            float shadow = (castShadows && receiveShadows) ?
                calculateParticlePointShadow(pointShadowMap, light.shadowIndex, FragPos, light.position.xyz, light.radius, N, light.shadowBias) : 0.0;

            vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.a * attenuation;
            Lo += (1.0 - shadow) * albedo * radiance * NdotL;
        }

        resultRGB = ambient + Lo;
    }

    resultRGB += (emmisiveColor * emmisiveIntensity);

    FragColor = vec4(resultRGB, alpha);
}
