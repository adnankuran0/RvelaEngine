#shader vertex
#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment
#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

uniform int u_Mode;
uniform int u_Layer;
uniform float u_Near;
uniform float u_Far;

layout(binding = 0) uniform sampler2D u_Texture2D;
layout(binding = 1) uniform sampler2DArray u_TextureArray;
layout(binding = 2) uniform usampler2D u_TextureUInt;

float LinearizeDepth(float depth, float near, float far)
{
    float z_ndc = depth * 2.0 - 1.0;
    float linearDepth = (2.0 * near * far) / (far + near - z_ndc * (far - near));
    return clamp((linearDepth - near) / (far - near), 0.0, 1.0);
}

void main()
{
    // Mode 0: None
    if (u_Mode == 0)
    {
        FragColor = texture(u_Texture2D, TexCoords);
    }
    // Mode 1: Normal Buffer ([-1, 1] mapped to [0, 1])
    else if (u_Mode == 1)
    {
        vec3 n = texture(u_Texture2D, TexCoords).xyz;
        if (dot(n, n) < 0.0001)
        {
            FragColor = vec4(0.06, 0.07, 0.09, 1.0);
        }
        else
        {
            vec3 visualNormal = normalize(n) * 0.5 + 0.5;
            FragColor = vec4(visualNormal, 1.0);
        }
    }
    // Mode 2: Depth Buffer (Linearized)
    else if (u_Mode == 2)
    {
        float d = texture(u_Texture2D, TexCoords).r;
        if (d >= 0.99999)
        {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
        else
        {
            float ld = LinearizeDepth(d, u_Near, u_Far);
            FragColor = vec4(vec3(1.0 - ld), 1.0);
        }
    }
    // Mode 3: Roughness
    else if (u_Mode == 3)
    {
        float r = texture(u_Texture2D, TexCoords).r;
        FragColor = vec4(vec3(r), 1.0);
    }
    // Mode 4: Metallic
    else if (u_Mode == 4)
    {
        float m = texture(u_Texture2D, TexCoords).r;
        FragColor = vec4(vec3(m), 1.0);
    }
    // Mode 5: SSAO
    else if (u_Mode == 5)
    {
        float ao = texture(u_Texture2D, TexCoords).r;
        FragColor = vec4(vec3(ao), 1.0);
    }
    // Mode 6: SSGI
    else if (u_Mode == 6)
    {
        vec3 gi = texture(u_Texture2D, TexCoords).rgb;
        FragColor = vec4(gi, 1.0);
    }
    // Mode 7: SSR
    else if (u_Mode == 7)
    {
        vec4 ssr = texture(u_Texture2D, TexCoords);
        FragColor = vec4(ssr.rgb, 1.0);
    }
    // Mode 8: Directional Shadow Map
    else if (u_Mode == 8)
    {
        float shadowDepth = texture(u_TextureArray, vec3(TexCoords, float(u_Layer))).r;
        FragColor = vec4(vec3(shadowDepth), 1.0);
    }
    // Mode 9: Spot Shadow Map
    else if (u_Mode == 9)
    {
        float shadowDepth = texture(u_TextureArray, vec3(TexCoords, float(u_Layer))).r;
        FragColor = vec4(vec3(shadowDepth), 1.0);
    }
    // Mode 10: Bloom Texture
    else if (u_Mode == 10)
    {
        vec3 bloom = texture(u_Texture2D, TexCoords).rgb;
        FragColor = vec4(bloom, 1.0);
    }
    // Mode 11: Bright Pass Texture
    else if (u_Mode == 11)
    {
        vec3 bright = texture(u_Texture2D, TexCoords).rgb;
        FragColor = vec4(bright, 1.0);
    }
    // Mode 12: Entity ID Buffer
    else if (u_Mode == 12)
    {
        uint id = texture(u_TextureUInt, TexCoords).r;
        if (id == 0u)
        {
            FragColor = vec4(0.04, 0.04, 0.05, 1.0);
        }
        else
        {
            float r = float((id * 97u) % 255u) / 255.0;
            float g = float((id * 191u + 37u) % 255u) / 255.0;
            float b = float((id * 313u + 83u) % 255u) / 255.0;
            FragColor = vec4(r, g, b, 1.0);
        }
    }
    // Mode 13: Screen Color
    else if (u_Mode == 13)
    {
        vec3 col = texture(u_Texture2D, TexCoords).rgb;
        FragColor = vec4(col, 1.0);
    }
    else
    {
        FragColor = texture(u_Texture2D, TexCoords);
    }
}
