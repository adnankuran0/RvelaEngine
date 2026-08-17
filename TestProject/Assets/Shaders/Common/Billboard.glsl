#ifndef BILLBOARD_GLSL
#define BILLBOARD_GLSL

struct BillboardTransform
{
    vec4 worldPos;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
};

BillboardTransform CalculateBillboard(int mode, mat4 modelMat, mat4 viewMat, vec3 aPos, vec3 camPosition)
{
    BillboardTransform result;

    if (mode == 0)
    {
        result.worldPos = modelMat * vec4(aPos, 1.0);
        result.normal = vec3(0.0, 1.0, 0.0);
        result.tangent = vec3(1.0, 0.0, 0.0);
        result.bitangent = vec3(0.0, 0.0, 1.0);
        return result;
    }

    vec3 scale = vec3(
        length(vec3(modelMat[0][0], modelMat[0][1], modelMat[0][2])),
        length(vec3(modelMat[1][0], modelMat[1][1], modelMat[1][2])),
        length(vec3(modelMat[2][0], modelMat[2][1], modelMat[2][2]))
    );

    vec3 objectCenter = modelMat[3].xyz;

    if (mode == 1)
    {
        vec3 camRight = vec3(viewMat[0][0], viewMat[1][0], viewMat[2][0]);
        vec3 camUp    = vec3(viewMat[0][1], viewMat[1][1], viewMat[2][1]);
        vec3 camDir   = vec3(viewMat[0][2], viewMat[1][2], viewMat[2][2]);

        vec3 worldVertexPos = objectCenter 
            + (camRight * (aPos.x * scale.x)) 
            + (camUp    * (aPos.y * scale.y)) 
            + (camDir   * (aPos.z * scale.z));

        result.worldPos = vec4(worldVertexPos, 1.0);
        result.normal = camDir;
        result.tangent = camRight;
        result.bitangent = camUp;
    }
    else
    {
        vec3 toCam = camPosition - objectCenter;
        toCam.y = 0.0;
        float len = length(toCam);
        vec3 forward = len > 0.0001 ? toCam / len : vec3(0.0, 0.0, 1.0);

        vec3 worldUp = vec3(0.0, 1.0, 0.0);
        vec3 right = normalize(cross(worldUp, forward));

        vec3 worldVertexPos = objectCenter 
            + (right   * (aPos.x * scale.x)) 
            + (worldUp * (aPos.y * scale.y)) 
            + (forward * (aPos.z * scale.z));

        result.worldPos = vec4(worldVertexPos, 1.0);
        result.normal = forward;
        result.tangent = right;
        result.bitangent = worldUp;
    }

    return result;
}

#endif