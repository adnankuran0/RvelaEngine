#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = lightSpaceMatrix * worldPos;
}

#shader fragment
#version 460 core
void main() {
}