#shader compute
#version 450 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D u_ScreenTexture;
uniform int u_MipLevel;

layout(std430, binding = 0) buffer LuminanceBuffer {
    float luminance;
};

void main() {
    vec3 color = texelFetch(u_ScreenTexture, ivec2(0, 0), u_MipLevel).rgb;
    luminance = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}
