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
    vec2 center = texelFetch(u_MaskTex, coord, 0).rg;
    
    if (center.g > 0.5 || center.r > 0.5) discard;
    
    ivec2 offsets[8] = ivec2[](
        ivec2(2, 0), ivec2(-2, 0), ivec2(0, 2), ivec2(0, -2),
        ivec2(1, 1), ivec2(-1, 1), ivec2(1, -1), ivec2(-1, -1)
    );
    
    bool isVisible = false;
    bool isOccluded = false;

    for (int i = 0; i < 8; i++) {
        vec2 val = texelFetch(u_MaskTex, coord + offsets[i], 0).rg;
        if (val.r > 0.5) {
            isVisible = true;
            break;
        } else if (val.g > 0.5) {
            isOccluded = true;
        }
    }

    if (isVisible) {
        FragColor = vec4(1.0, 0.7, 0.1, 1.0);
    } else if (isOccluded) {
        FragColor = vec4(1.0, 0.7, 0.1, 0.32);
    } else {
        discard;
    }
}