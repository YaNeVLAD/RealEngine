#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in float a_TexIndex;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_BoneWeights;
layout (location = 7) in vec4 a_Tangent;

// === БЕЗЛИМИТНЫЙ БУФЕР КОСТЕЙ ===
layout (std430, binding = 4) readonly buffer BoneBuffer
{
    mat4 u_Bones[];
};

uniform mat4 u_ModelViewProjection;
uniform mat4 u_ModelMatrix;
uniform mat3 u_NormalMatrix;

out vec4 v_Color;
out vec2 v_TexCoord;
out vec3 v_Normal;
out vec3 v_FragmentWorldPos;
out mat3 v_TBN;

void main() {
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;

    mat4 boneTransform = mat4(0.0);
    float totalWeight = a_BoneWeights[0] + a_BoneWeights[1] + a_BoneWeights[2] + a_BoneWeights[3];

    if (totalWeight > 0.01) {
        vec4 w = a_BoneWeights / totalWeight;

        boneTransform += u_Bones[a_BoneIDs[0]] * w[0];
        boneTransform += u_Bones[a_BoneIDs[1]] * w[1];
        boneTransform += u_Bones[a_BoneIDs[2]] * w[2];
        boneTransform += u_Bones[a_BoneIDs[3]] * w[3];
    } else {
        boneTransform = mat4(1.0);
    }

    vec4 localPos = boneTransform * vec4(a_Position, 1.0);
    localPos.w = 1.0;

    vec4 worldPos = u_ModelMatrix * localPos;
    v_FragmentWorldPos = vec3(worldPos);
    gl_Position = u_ModelViewProjection * localPos;

    mat3 boneNormalMatrix = mat3(boneTransform);
    vec3 localNormal = boneNormalMatrix * a_Normal;
    v_Normal = u_NormalMatrix * localNormal;

    vec3 localTangent = boneNormalMatrix * a_Tangent.xyz;
    vec3 tangent = normalize(u_NormalMatrix * localTangent);
    vec3 normal = normalize(v_Normal);

    if (length(tangent) > 0.0001) {
        tangent = normalize(tangent - dot(tangent, normal) * normal);
        vec3 bitangent = cross(normal, tangent) * a_Tangent.w;
        v_TBN = mat3(tangent, bitangent, normal);
    } else {
        vec3 up = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        vec3 t_fallback = normalize(cross(up, normal));
        vec3 b_fallback = cross(normal, t_fallback);
        v_TBN = mat3(t_fallback, b_fallback, normal);
    }
}