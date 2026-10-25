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

layout(binding = 0) uniform sampler2D u_MaskColorTex;
layout(binding = 1) uniform sampler2D u_MaskInfoTex;
layout(binding = 2) uniform sampler2D u_SceneDepthTex;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec4 centerCol = texelFetch(u_MaskColorTex, coord, 0);

    if (centerCol.a > 0.001) {
        discard;
    }

    float sceneZ = texelFetch(u_SceneDepthTex, coord, 0).r;

    const int MAX_R = 8;
    vec4 bestColor = vec4(0.0);
    float bestAlpha = 0.0;

    for (int y = -MAX_R; y <= MAX_R; ++y) {
        for (int x = -MAX_R; x <= MAX_R; ++x) {
            if (x == 0 && y == 0) continue;
            float dist = length(vec2(x, y));
            if (dist > float(MAX_R)) continue;

            ivec2 sampleCoord = coord + ivec2(x, y);
            vec4 sCol = texelFetch(u_MaskColorTex, sampleCoord, 0);
            if (sCol.a > 0.001) {
                vec4 sInfo = texelFetch(u_MaskInfoTex, sampleCoord, 0);
                float sWidth = sInfo.r;
                float isXRay = sInfo.g;
                float maskZ = sInfo.b;

                if (isXRay < 0.5) {
                    if (sceneZ < maskZ - 0.0002) {
                        continue;
                    }
                }

                if (dist <= sWidth) {
                    float edge = clamp(sWidth - dist + 0.5, 0.0, 1.0);
                    float a = sCol.a * edge;
                    if (a > bestAlpha) {
                        bestAlpha = a;
                        bestColor = vec4(sCol.rgb, a);
                    }
                }
            }
        }
    }

    if (bestAlpha <= 0.001) {
        discard;
    }

    FragColor = bestColor;
}
