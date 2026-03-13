#shader vertex
#version 460

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uMVP;

out vec4 Color;

void main()
{
    vec4 pos = uMVP * vec4(aPosition, 1.0);
    pos.z -= 0.0001 * pos.w;  // z fighting fix
    gl_Position = pos;
    Color = aColor;
}

#shader fragment
#version 460

in vec4 Color;
out vec4 FragColor;

void main()
{
    FragColor = Color;

}
