#shader vertex
#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment
#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D hdrTexture;
uniform float threshold;
uniform float knee;

float brightness(vec3 color) {
    return max(color.r, max(color.g, color.b));
}

void main()
{
    vec3 color = textureLod(hdrTexture, TexCoords, 0.0).rgb;
    
    float br = brightness(color);
    if (br < threshold - knee) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float softFactor = smoothstep(threshold - knee, threshold, br);
    float contribution = max(br - threshold + knee * softFactor, 0.0);

    vec3 result = color * (contribution / max(br, 1e-5));
    FragColor = vec4(result, 1.0);
}