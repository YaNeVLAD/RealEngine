#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in float a_TexIndex;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_BoneWeights;
layout (location = 7) in vec4 a_Tangent;

layout (location = 8) in mat4 a_InstanceMatrix;
layout (location = 12) in mat4 a_NormalMatrix;

uniform mat4 u_ViewProjection;
uniform bool u_HasNonUniformScale;

out vec4 v_Color;
out vec2 v_TexCoord;
out vec3 v_Normal;
out vec3 v_FragmentWorldPos;
out mat3 v_TBN;

void main()
{
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;

    mat3 normalMatrix;
    if (u_HasNonUniformScale)
    {
        normalMatrix = mat3(a_NormalMatrix);
    }
    else
    {
        normalMatrix = mat3(a_InstanceMatrix);
    }
    v_Normal = normalMatrix * a_Normal;

    vec4 worldPos = a_InstanceMatrix * vec4(a_Position, 1.0);
    v_FragmentWorldPos = vec3(worldPos);
    gl_Position = u_ViewProjection * worldPos;

    vec3 tangent = normalMatrix * a_Tangent.xyz;
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