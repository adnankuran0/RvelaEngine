#version 460 core
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_AMD_vertex_shader_layer : enable
#extension GL_NV_viewport_array2 : enable

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 shadowMatrices[6];
uniform int baseLayer;

out vec4 FragPos;

void main()
{
    int face = gl_InstanceID;
    FragPos = model * vec4(aPos, 1.0);
    gl_Position = shadowMatrices[face] * FragPos;
    gl_Layer = baseLayer + face;
}