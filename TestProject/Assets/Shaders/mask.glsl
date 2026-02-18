#shader vertex
#version 460 core
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core
uniform vec4 u_Color;
layout(location = 0) out vec4 FragColor;
void main() {
    FragColor = u_Color; 
}