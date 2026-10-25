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

uniform int u_Radius = 2;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec4 centerCol = texelFetch(u_MaskColorTex, coord, 0);

    if (centerCol.a > 0.001) {
        discard;
    }

    int r = clamp(u_Radius, 1, 8);

    ivec2 offsets[12];
    int count = 0;

    offsets[count++] = ivec2( r,  0);
    offsets[count++] = ivec2(-r,  0);
    offsets[count++] = ivec2( 0,  r);
    offsets[count++] = ivec2( 0, -r);
    offsets[count++] = ivec2( r,  r);
    offsets[count++] = ivec2(-r,  r);
    offsets[count++] = ivec2( r, -r);
    offsets[count++] = ivec2(-r, -r);

    if (r > 1) {
        offsets[count++] = ivec2( 1,  0);
        offsets[count++] = ivec2(-1,  0);
        offsets[count++] = ivec2( 0,  1);
        offsets[count++] = ivec2( 0, -1);
    }

    float sceneZ = -1.0;
    vec4 bestColor = vec4(0.0);
    bool found = false;

    for (int i = 0; i < count; ++i) {
        ivec2 sampleCoord = coord + offsets[i];
        vec4 sCol = texelFetch(u_MaskColorTex, sampleCoord, 0);
        if (sCol.a > 0.001) {
            vec4 sInfo = texelFetch(u_MaskInfoTex, sampleCoord, 0);
            float isXRay = sInfo.g;
            float maskZ = sInfo.b;

            if (isXRay < 0.5) {
                if (sceneZ < 0.0) {
                    sceneZ = texelFetch(u_SceneDepthTex, coord, 0).r;
                }
                if (sceneZ < maskZ - 0.0005) {
                    continue;
                }
            }

            bestColor = sCol;
            found = true;
            break;
        }
    }

    if (!found) {
        discard;
    }

    FragColor = bestColor;
}
