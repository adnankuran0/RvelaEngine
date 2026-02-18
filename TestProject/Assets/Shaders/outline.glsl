#shader vertex
#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;
out vec2 TexCoords;
void main() {
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

#shader fragment
#version 460 core
layout(location = 0) out vec4 FragColor;

uniform sampler2D u_MaskTex;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    float center = texelFetch(u_MaskTex, coord, 0).r;
    
    if (center > 0.5) discard;
    
    ivec2 offsets[4] = ivec2[](
        ivec2(2, 0), ivec2(-2, 0), ivec2(0, 2), ivec2(0, -2)
    );
    
    for (int i = 0; i < 4; i++) {
        if (texelFetch(u_MaskTex, coord + offsets[i], 0).r > 0.5) {
            FragColor = vec4(1.0, 0.7, 0.1, 1.0);
            return;
        }
    }
    
    discard;
}