#shader vertex
#version 460 core

layout (location = 0) in vec3 aPosition;

uniform mat4 u_Projection;

void main()
{
    gl_Position = u_Projection * vec4(aPosition, 1.0);
}

#shader fragment
#version 460 core

layout (location = 0) out uint EntityID;

uniform uint u_EntityID;

void main()
{
    EntityID = u_EntityID;
}
