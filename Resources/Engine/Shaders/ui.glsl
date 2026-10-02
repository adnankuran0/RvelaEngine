#shader vertex
#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aUseTexture;
layout (location = 4) in vec2 aQuadSize;
layout (location = 5) in vec2 aLocalPos;
layout (location = 6) in float aCornerRadius;

uniform mat4 u_Projection;

out vec4 vColor;
out vec2 vTexCoord;
out float vUseTexture;
out vec2 vQuadSize;
out vec2 vLocalPos;
out float vCornerRadius;

void main()
{
    vColor = aColor;
    vTexCoord = aTexCoord;
    vUseTexture = aUseTexture;
    vQuadSize = aQuadSize;
    vLocalPos = aLocalPos;
    vCornerRadius = aCornerRadius;
    gl_Position = u_Projection * vec4(aPosition, 1.0);
}

#shader fragment
#version 460 core

in vec4 vColor;
in vec2 vTexCoord;
in float vUseTexture;
in vec2 vQuadSize;
in vec2 vLocalPos;
in float vCornerRadius;

uniform sampler2D u_Texture;

out vec4 FragColor;

float sdRoundedBox(vec2 p, vec2 b, float r)
{
    vec2 q = abs(p) - b + vec2(r);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main()
{
    vec4 finalColor = vec4(1.0);

    if (vUseTexture > 1.5)
    {
        float distance = texture(u_Texture, vTexCoord).a;
        float width = fwidth(distance);
        if (width < 0.0001) width = 0.05;

        float threshold = 0.5 - vCornerRadius;
        float alpha = smoothstep(threshold - width, threshold + width, distance);
        finalColor = vec4(vColor.rgb, vColor.a * alpha);
    }
    else if (vUseTexture > 0.5)
    {
        finalColor = texture(u_Texture, vTexCoord) * vColor;
    }
    else
    {
        finalColor = vColor;
    }

    if (vCornerRadius > 0.5 && vUseTexture < 1.5)
    {
        vec2 halfSize = vQuadSize * 0.5;
        float radius = min(vCornerRadius, min(halfSize.x, halfSize.y));
        float dist = sdRoundedBox(vLocalPos, halfSize, radius);
        float alpha = 1.0 - smoothstep(-1.0, 1.0, dist);
        finalColor.a *= alpha;
    }

    if (finalColor.a < 0.01)
    {
        discard;
    }

    FragColor = finalColor;
}
