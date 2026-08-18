layout(std140, binding = 0) uniform CameraData
{
    mat4 view;
    mat4 projection;
    mat4 invView;
    mat4 invProjection;

    vec3 camPos;
    float _pad0;

    vec2 windowSize;
    float nearPlane;
    float farPlane;
};