#shader vertex
#version 460 core
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_AMD_vertex_shader_layer : enable
#extension GL_NV_viewport_array2 : enable

layout (location = 0) in vec3 aPos;
layout (location = 3) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 shadowMatrix;  
uniform int baseLayer;
uniform int currentFace;    
uniform vec2 UVScale;
uniform vec2 UVOffset;

uniform int billboardMode;
uniform mat4 cameraView;
uniform vec3 camPos;

out vec4 FragPos;
out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords * UVScale + UVOffset;

    vec4 worldPos;

    if (billboardMode == 0)
    {
        worldPos = model * vec4(aPos, 1.0);
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
            vec3 camRight = vec3(cameraView[0][0], cameraView[1][0], cameraView[2][0]);
            vec3 camUp    = vec3(cameraView[0][1], cameraView[1][1], cameraView[2][1]);
            vec3 camDir   = vec3(cameraView[0][2], cameraView[1][2], cameraView[2][2]);

            vec3 worldVertexPos = objectCenter 
                + (camRight * (aPos.x * scale.x)) 
                + (camUp    * (aPos.y * scale.y))
                + (camDir   * (aPos.z * scale.z));

            worldPos = vec4(worldVertexPos, 1.0);
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
        }
    }

    FragPos = worldPos;
    gl_Position = shadowMatrix * FragPos;
    gl_Layer = baseLayer + currentFace;
}

#shader fragment
#version 460 core 
in vec4 FragPos;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D albedoMap;
uniform bool useAlbedoMap;
uniform vec4 albedoColor;
uniform int transparencyMode;
uniform float alphaCutoff;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    if (transparencyMode == 2) {
        float alpha = (useAlbedoMap ? texture(albedoMap, TexCoords).a : 1.0) * albedoColor.a;
        if (alpha < alphaCutoff)
            discard;
    }

    float lightDistance = length(FragPos.xyz - lightPos);
    lightDistance = lightDistance / far_plane;
    gl_FragDepth = lightDistance;
}