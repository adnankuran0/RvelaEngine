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

float brightness(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722)); 
}

void main()
{
    vec3 color = texture(hdrTexture, TexCoords).rgb;
    float br = brightness(color);

    float soft = clamp((br - threshold + knee) / (2.0 * knee), 0.0, 1.0);
    float contrib = max(br - threshold, 0.0) + soft * knee;

    vec4 finalColor = vec4(color * (contrib / max(br, 1e-5)), 1.0);

    FragColor = finalColor;
}
