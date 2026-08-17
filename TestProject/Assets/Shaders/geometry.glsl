#shader vertex
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBitangent;
layout(location = 4) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform vec2 UVScale;
uniform vec2 UVOffset;

uniform int billboardMode;
uniform vec3 camPos;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
    vec4 worldPos;
    vec3 calculatedViewNormal;

    if (billboardMode == 0)
    {
        worldPos = model * vec4(aPos, 1.0);
        calculatedViewNormal = mat3(view) * (normalMatrix * aNormal);
    }
    else
    {
        vec3 scale = vec3(
            length(vec3(model[0][0], model[0][1], model[0][2])),
            length(vec3(model[1][0], model[1][1], model[1][2])),
            length(vec3(model[2][0], model[2][1], model[2][2]))
        );

        vec3 objectCenter = model[3].xyz;

        if (billboardMode == 1) // Spherical
        {
            vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
            vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);
            vec3 camDir   = vec3(view[0][2], view[1][2], view[2][2]);

            vec3 worldVertexPos = objectCenter 
                + (camRight * (aPos.x * scale.x)) 
                + (camUp    * (aPos.y * scale.y))
                + (camDir   * (aPos.z * scale.z));

            worldPos = vec4(worldVertexPos, 1.0);
            
            calculatedViewNormal = vec3(0.0, 0.0, 1.0);
        }
        else // Cylindrical
        {
            vec3 toCam = camPos - objectCenter;
            toCam.y = 0.0;
            float len = length(toCam);
            vec3 forward = len > 0.0001 ? toCam / len : vec3(0.0, 0.0, 1.0);

            vec3 worldUp = vec3(0.0, 1.0, 0.0);
            vec3 right = normalize(cross(worldUp, forward));

            vec3 worldVertexPos = objectCenter 
                + (right   * (aPos.x * scale.x)) 
                + (worldUp * (aPos.y * scale.y))
                + (forward * (aPos.z * scale.z));

            worldPos = vec4(worldVertexPos, 1.0);
            
            calculatedViewNormal = mat3(view) * forward;
        }
    }

    FragPos = vec3(view * worldPos);
    Normal = calculatedViewNormal;
    TexCoords = aTexCoords * UVScale + UVOffset;
    gl_Position = projection * view * worldPos;
}

#shader fragment
#version 460 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

layout(location = 0) out vec3 gNormal;
layout(location = 1) out float gRoughness;
layout(location = 2) out float gMetallic;

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D roughnessMap;
layout(binding = 2) uniform sampler2D metallicMap;

uniform bool useAlbedoMap;
uniform bool useRoughnessMap;
uniform bool useMetallicMap;

uniform vec4 albedoColor;
uniform float roughness;
uniform float metallic;

uniform int transparencyMode;
uniform float alphaCutoff;

void main()
{
    if (transparencyMode == 2) // alpha scissor
    {
        float alpha = (useAlbedoMap ? texture(albedoMap, TexCoords).a : 1.0) * albedoColor.a;
        if (alpha < alphaCutoff)
            discard;
    }

    gNormal = normalize(Normal);
    gRoughness = useRoughnessMap ? texture(roughnessMap, TexCoords).r : roughness;
    gMetallic = useMetallicMap ? texture(metallicMap, TexCoords).r : metallic;
}