#version 430 core

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 0) readonly buffer InputInstances { mat4 inputTransforms[]; };
layout (std430, binding = 1) writeonly buffer OutputInstances { mat4 outputTransforms[]; };
layout (std430, binding = 3) writeonly buffer OutputNormals { mat4 outputNormalMatrices[]; };

struct DrawCommand
{
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
};

layout (std430, binding = 2) buffer CommandBuffer { DrawCommand cmd; };

uniform vec4 u_FrustumPlanes[6];
uniform uint u_TotalInstances;
uniform float u_MeshRadius;

uniform vec3 u_CameraPos;
uniform float u_MaxDistance;

uniform bool u_ComputeNormals;

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= u_TotalInstances) return;

    mat4 model = inputTransforms[idx];
    vec3 pos = vec3(model[3]);

    float maxScale = max(max(length(vec3(model[0])), length(vec3(model[1]))), length(vec3(model[2])));
    float radius = u_MeshRadius * maxScale;

    // DISTANCE CULLING
    if ((distance(u_CameraPos, pos) - radius) > u_MaxDistance) return;

    // FRUSTUM CULLING
    bool visible = true;
    for (int i = 0; i < 6; i++)
    {
        if (dot(u_FrustumPlanes[i].xyz, pos) + u_FrustumPlanes[i].w < -radius)
        {
            visible = false;
            break;
        }
    }

    if (visible)
    {
        uint outIdx = atomicAdd(cmd.instanceCount, 1);
        outputTransforms[outIdx] = model;
        if (u_ComputeNormals)
        {
            outputNormalMatrices[outIdx] = mat4(transpose(inverse(mat3(model))));
        }
    }
}