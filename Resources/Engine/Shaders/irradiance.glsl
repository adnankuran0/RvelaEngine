#shader vertex
#version 460 core
layout(location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 projection;
uniform mat4 view;

void main() 
{
    localPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core
out vec4 FragColor;
in vec3 localPos;

#include "Common/Constants.glsl"

uniform samplerCube environmentMap;

void main() 
{
    vec3 normal = normalize(localPos);
    vec3 irradiance = vec3(0.0);
    
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));
    
    float sampleDelta = 0.025;
    int nrSamples = 0;
    
    for(float phi = 0.0; phi < TWO_PI; phi += sampleDelta) 
    {
        for(float theta = 0.0; theta < HALF_PI; theta += sampleDelta) 
        {
            vec3 tangentSample = vec3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );
            
            vec3 sampleDir = tangentSample.x * right + 
                             tangentSample.y * up + 
                             tangentSample.z * normal;
            
            vec3 sampleColor = texture(environmentMap, sampleDir).rgb;
            
            irradiance += sampleColor * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    
    irradiance = PI * irradiance / float(nrSamples);
    
    FragColor = vec4(irradiance, 1.0);
}