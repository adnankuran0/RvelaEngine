#version 400 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D hdrTexture;
uniform float threshold;
uniform float knee;

float brightness(vec3 color) {
    return max(max(color.r, color.g), color.b);
}

void main()
{
    vec3 color = texture(hdrTexture, TexCoords).rgb;
    float br = brightness(color);

    float soft = threshold - knee;
    float softFactor = clamp((br - soft) / (knee + 1e-5), 0.0, 1.0);
    float contribution = max(br - threshold + knee * softFactor, 0.0);

    vec3 result = color * (contribution / max(br, 1e-5));
    FragColor = vec4(result, 1.0);
}