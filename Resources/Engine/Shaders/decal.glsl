#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;

#include "Common/Camera.glsl"

uniform mat4 u_Model;

void main()
{
    gl_Position = projection * view * u_Model * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core

layout(location = 0) out vec4 FragColor;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"
#include "Common/Lights.glsl"
#include "Common/Shadows.glsl"

layout(binding = 0) uniform sampler2D u_DepthTexture;
layout(binding = 1) uniform sampler2D u_NormalTexture;
layout(binding = 2) uniform sampler2D u_DecalTexture;

layout(binding = 6) uniform sampler2DArray shadowMap;
layout(binding = 7) uniform samplerCubeArray pointShadowMap;
layout(binding = 8) uniform samplerCube irradianceMap;

uniform mat4 u_Model;
uniform mat4 u_InvModel;
uniform vec4 u_Color;
uniform float u_Opacity;
uniform float u_AngleCutoff;
uniform bool u_HasTexture;

uniform bool u_Lit;
uniform bool u_ReceiveShadows;
uniform vec3 ambientColor;
uniform float ambientIntensity;
uniform bool useIBL;
uniform float iblIntensity;

uniform float u_UpperFade;
uniform float u_LowerFade;

uniform bool u_DistanceFade;
uniform float u_FadeStart;
uniform float u_FadeEnd;

void main()
{
    vec2 screenUV = gl_FragCoord.xy / windowSize;

    float depth = texture(u_DepthTexture, screenUV).r;
    if (depth >= 0.999999)
    {
        discard;
    }

    vec3 viewPos = ReconstructViewPos(screenUV, depth, invProjection);
    vec3 worldPos = (invView * vec4(viewPos, 1.0)).xyz;

    vec3 localPos = (u_InvModel * vec4(worldPos, 1.0)).xyz;
    if (any(greaterThan(abs(localPos), vec3(0.5))))
    {
        discard;
    }

    vec2 decalUV = vec2(localPos.x + 0.5, localPos.y + 0.5);

    vec4 texColor = u_HasTexture ? texture(u_DecalTexture, decalUV) : vec4(1.0);
    vec4 finalColor = texColor * u_Color;
    finalColor.a *= u_Opacity;

    vec3 viewNormal = texture(u_NormalTexture, screenUV).rgb;
    vec3 worldNormal = vec3(0.0, 1.0, 0.0);
    if (length(viewNormal) > 0.01)
    {
        vec3 normVN = normalize(viewNormal);
        worldNormal = normalize(mat3(invView) * normVN);

        vec3 decalForwardView = normalize((view * u_Model * vec4(0.0, 0.0, -1.0, 0.0)).xyz);
        float NdotD = dot(normVN, -decalForwardView);

        if (u_AngleCutoff > 0.0)
        {
            float minCos = cos(radians(u_AngleCutoff));
            float angleFade = smoothstep(minCos * 0.7, minCos, NdotD);
            finalColor.a *= angleFade;
        }
    }

    if (u_UpperFade > 0.001)
    {
        float upperLimit = 0.5;
        float upperStart = 0.5 - clamp(u_UpperFade, 0.0, 0.5);
        float zFade = smoothstep(upperLimit, upperStart, localPos.z);
        finalColor.a *= zFade;
    }
    if (u_LowerFade > 0.001)
    {
        float lowerLimit = -0.5;
        float lowerStart = -0.5 + clamp(u_LowerFade, 0.0, 0.5);
        float zFade = smoothstep(lowerLimit, lowerStart, localPos.z);
        finalColor.a *= zFade;
    }

    if (u_DistanceFade && u_FadeEnd > u_FadeStart)
    {
        float camDist = length(worldPos - camPos);
        float distFade = smoothstep(u_FadeEnd, u_FadeStart, camDist);
        finalColor.a *= distFade;
    }

    if (finalColor.a <= 0.001)
    {
        discard;
    }

    if (u_Lit)
    {
        vec3 Lo = vec3(0.0);
        vec3 albedo = finalColor.rgb;

        if (hasDirectionalLight == 1)
        {
            vec3 L = normalize(-directionalLight.direction.xyz);
            float NdotL = max(dot(worldNormal, L), 0.0);

            bool castShadows = directionalLight.direction.w > 0.5;
            float viewDepth = abs((view * vec4(worldPos, 1.0)).z);
            float shadow = (castShadows && u_ReceiveShadows) ?
                calculateDirectionalShadow(shadowMap, worldPos, worldNormal, L, viewDepth, directionalLight.shadowBias, directionalLight.normalBias, directionalLight.blurRadius) : 0.0;

            vec3 lightColor = directionalLight.colorIntensity.rgb;
            float lightIntensity = directionalLight.colorIntensity.a;

            Lo += (1.0 - shadow) * albedo * lightColor * lightIntensity * NdotL;
        }

        for (int i = 0; i < pointLightCount; ++i)
        {
            PointLight light = pointLights[i];
            vec3 L_vec = light.position.xyz - worldPos;
            float dist2 = dot(L_vec, L_vec);
            float radius2 = light.radius * light.radius;

            if (dist2 > radius2) continue;

            float distance = sqrt(dist2);
            vec3 L = L_vec / distance;

            float num = clamp(1.0 - (dist2 / radius2) * (dist2 / radius2), 0.0, 1.0);
            float attenuation = (num * num) / (dist2 + 1.0);

            float NdotL = max(dot(worldNormal, L), 0.0);

            bool castShadows = light.shadowIndex >= 0;
            float shadow = (castShadows && u_ReceiveShadows) ?
                calculatePointLightShadow(pointShadowMap, light.shadowIndex, worldPos, light.position.xyz, light.radius, worldNormal, light.shadowBias, light.blurRadius) : 0.0;

            vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.a * attenuation;
            Lo += (1.0 - shadow) * albedo * radiance * NdotL;
        }

        vec3 ambient = vec3(0.0);
        if (useIBL)
        {
            vec3 irradiance = texture(irradianceMap, worldNormal).rgb;
            ambient = irradiance * albedo * iblIntensity;
        }
        else
        {
            ambient = ambientColor * ambientIntensity * albedo;
        }

        finalColor.rgb = ambient + Lo;
    }

    FragColor = finalColor;
}
