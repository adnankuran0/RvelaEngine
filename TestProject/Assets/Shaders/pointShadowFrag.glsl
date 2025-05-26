#version 330 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPos);
    lightDistance = lightDistance / farPlane; // [0, 1] aralığına normalize et
    gl_FragDepth = lightDistance;
}