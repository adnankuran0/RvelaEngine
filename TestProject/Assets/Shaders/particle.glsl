#shader vertex
#version 460 core
layout(location = 0) in vec3 aPos;
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

void main()
{
    ParticleInstanceData data = instances[gl_InstanceID + instanceOffset];
    
    vec3 instPos = data.positionAndScale.xyz;
    float scale = data.positionAndScale.w;
    float rot = data.rotationAndCustom.x;
    
    ParticleColor = data.color;
    TexCoords = (aTexCoords * UVScale) + UVOffset;

    vec3 vertexPos;

    if (billboardMode == 1) 
    {
        vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);
        
        float cosRot = cos(rot);
        float sinRot = sin(rot);
        vec3 right = camRight * cosRot - camUp * sinRot;
        vec3 up    = camRight * sinRot + camUp * cosRot;
        
        vertexPos = instPos + (right * aPos.x + up * aPos.y + camRight * aPos.z) * scale;
    }
    else 
    {
        vertexPos = instPos + (aPos * scale);
    }

    gl_Position = projection * view * vec4(vertexPos, 1.0);
}

#shader fragment
#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec4 ParticleColor;

layout(binding = 0) uniform sampler2D albedoMap;

uniform bool useAlbedoMap;
uniform vec4 albedoColor;

uniform vec3 emmisiveColor;
uniform float emmisiveIntensity;

uniform int transparencyMode;
uniform float alphaCutoff;

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

    vec3 resultRGB = finalColor.rgb + (emmisiveColor * emmisiveIntensity);

    if(alpha < 0.001)
        discard;
        
    FragColor = vec4(resultRGB, alpha);
}