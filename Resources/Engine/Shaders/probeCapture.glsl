#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec2 aTexCoords;

uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Model;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec3 v_LocalPos;
out vec3 v_LocalNormal;
out vec2 v_TexCoords;

void main()
{
    vec4 worldPos = u_Model * vec4(aPos, 1.0);
    v_FragPos = worldPos.xyz;
    v_Normal = mat3(transpose(inverse(u_Model))) * aNormal;
    v_LocalPos = aPos;
    v_LocalNormal = aNormal;
    v_TexCoords = aTexCoords;
    gl_Position = u_Projection * u_View * worldPos;
}

#shader fragment
#version 460 core

out vec4 FragColor;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec3 v_LocalPos;
in vec3 v_LocalNormal;
in vec2 v_TexCoords;

uniform vec4 u_AlbedoColor;
uniform bool u_UseAlbedoMap;
layout(binding = 0) uniform sampler2D u_AlbedoMap;

uniform vec2 u_UVScale;
uniform vec2 u_UVOffset;
uniform bool u_UseTriplanar;
uniform bool u_UseWorldTriplanar;
uniform float u_TriplanarSharpness;

uniform vec3 u_EmissiveColor;
uniform float u_EmissiveIntensity;

uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;

struct CapturePointLight {
    vec4 positionRadius;
    vec4 colorIntensity;
};

uniform int u_PointLightCount;
uniform CapturePointLight u_PointLights[16];

vec3 getTriplanarWeights(vec3 normal, float sharpness)
{
    vec3 w = pow(abs(normal), vec3(sharpness));
    return w / max(0.00001, (w.x + w.y + w.z));
}

vec4 sampleTriplanar(sampler2D tex, vec3 p, vec3 weights)
{
    if (weights.y > 0.95) return texture(tex, p.xz);
    if (weights.x > 0.95) return texture(tex, p.zy * vec2(-1.0, 1.0));
    if (weights.z > 0.95) return texture(tex, p.xy);

    vec4 sampX = (weights.x > 0.001) ? texture(tex, p.zy * vec2(-1.0, 1.0)) * weights.x : vec4(0.0);
    vec4 sampY = (weights.y > 0.001) ? texture(tex, p.xz) * weights.y : vec4(0.0);
    vec4 sampZ = (weights.z > 0.001) ? texture(tex, p.xy) * weights.z : vec4(0.0);
    return sampX + sampY + sampZ;
}

void main()
{
    vec4 baseColor = u_AlbedoColor;
    if (u_UseAlbedoMap)
    {
        if (u_UseTriplanar)
        {
            vec3 triPos = u_UseWorldTriplanar ? v_FragPos : v_LocalPos;
            vec3 triNorm = u_UseWorldTriplanar ? normalize(v_Normal) : normalize(v_LocalNormal);
            vec3 triP = triPos * u_UVScale.x + vec3(u_UVOffset.x, u_UVOffset.y, 0.0);
            vec3 weights = getTriplanarWeights(triNorm, u_TriplanarSharpness);
            baseColor *= sampleTriplanar(u_AlbedoMap, triP, weights);
        }
        else
        {
            vec2 uv = v_TexCoords * u_UVScale + u_UVOffset;
            baseColor *= texture(u_AlbedoMap, uv);
        }
    }

    vec3 N = normalize(v_Normal);
    vec3 diffuseLight = vec3(0.0);

    vec3 L_dir = normalize(-u_LightDir);
    float diffDir = max(dot(N, L_dir), 0.0);
    diffuseLight += diffDir * u_LightColor;

    for (int i = 0; i < u_PointLightCount; ++i)
    {
        vec3 lightPos = u_PointLights[i].positionRadius.xyz;
        float radius = u_PointLights[i].positionRadius.w;
        vec3 lightCol = u_PointLights[i].colorIntensity.rgb * u_PointLights[i].colorIntensity.w;

        vec3 L_vec = lightPos - v_FragPos;
        float dist2 = dot(L_vec, L_vec);
        float radius2 = radius * radius;
        if (dist2 > radius2) continue;

        float dist = sqrt(dist2);
        vec3 L = L_vec / dist;
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0) continue;

        float num = clamp(1.0 - (dist2 / radius2) * (dist2 / radius2), 0.0, 1.0);
        float atten = (num * num) / (dist2 + 1.0);
        diffuseLight += NdotL * lightCol * atten;
    }

    vec3 ambient = u_AmbientColor;
    vec3 color = (ambient + diffuseLight) * baseColor.rgb + (u_EmissiveColor * u_EmissiveIntensity);

    FragColor = vec4(color, 1.0);
}
