#shader vertex
#version 460 core
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_AMD_vertex_shader_layer : enable
#extension GL_NV_viewport_array2 : enable

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 shadowMatrix;  
uniform int baseLayer;
uniform int currentFace;    

out vec4 FragPos;

void main()
{
    FragPos = model * vec4(aPos, 1.0);
    gl_Position = shadowMatrix * FragPos;
    gl_Layer = baseLayer + currentFace;
}

#shader fragment
#version 460 core 
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPos);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / far_plane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}