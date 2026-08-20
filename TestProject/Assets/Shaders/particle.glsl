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

layout(binding = 0) uniform sampler2D albedoMap;

uniform bool useAlbedoMap;
uniform vec4 albedoColor;

uniform vec3 emmisiveColor;
uniform float emmisiveIntensity;

uniform int transparencyMode;
uniform float alphaCutoff;

uniform int shadingMode;
uniform vec3 ambientColor;
uniform float ambientIntensity;

void main()
{
    vec4 texColor = useAlbedoMap ? texture(albedoMap, TexCoords) : vec4(1.0);
    vec4 finalColor = texColor * albedoColor * ParticleColor;
    
    float alpha = finalColor.a;

    if (transparencyMode == 0) 
    {
        alpha = 1.0;
    } 
    else if (transparencyMode == 2) 
    {
        if (alpha < alphaCutoff) discard;
        alpha = 1.0; 
    }

    if(alpha < 0.001)
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
            float NdotL = max(dot(N, L), 0.0);
            
            vec3 lightColor = directionalLight.colorIntensity.rgb;
            float lightIntensity = directionalLight.colorIntensity.a;

            Lo += albedo * lightColor * lightIntensity * NdotL;
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
            
            float NdotL = max(dot(N, L), 0.0);
            vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.a * attenuation;
            Lo += albedo * radiance * NdotL;
        }

        resultRGB = ambient + Lo;
    }

    resultRGB += (emmisiveColor * emmisiveIntensity);

    FragColor = vec4(resultRGB, alpha);
}