#version 450 core

layout (location = 0) out vec4 o_Color;

in vec4 v_Color;
in vec2 v_TexCoord;
in vec3 v_Normal;
in vec3 v_FragmentWorldPos;
in mat3 v_TBN;

uniform vec3 u_AlbedoColor;
uniform vec3 u_EmissionColor;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;

uniform sampler2D u_AlbedoMap;             uniform int u_HasAlbedoMap;
uniform sampler2D u_NormalMap;             uniform int u_HasNormalMap;
uniform sampler2D u_EmissionMap;           uniform int u_HasEmissionMap;
uniform sampler2D u_MetallicRoughnessMap;  uniform int u_HasMetallicRoughnessMap;
uniform sampler2D u_AOMap;                 uniform int u_HasAOMap;

uniform samplerCube u_IrradianceMap;
uniform int u_HasIrradianceMap;

uniform int u_LightType;
uniform vec3 u_LightPos;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_LightAmbient;
uniform float u_LightConstant;
uniform float u_LightLinear;
uniform float u_LightQuadratic;
uniform float u_LightCutOff;
uniform float u_LightExponent;

uniform vec3 u_CameraPos;

const float PI = 3.14159265359;

vec3 sRGBToLinear(vec3 srgbIn) { return pow(srgbIn, vec3(2.2)); }
vec3 linearToSRGB(vec3 linearIn) { return pow(linearIn, vec3(1.0 / 2.2)); }

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec4 albedoTex = u_HasAlbedoMap != 0 ? texture(u_AlbedoMap, v_TexCoord) : vec4(1.0);
    vec3 albedo = albedoTex.rgb * sRGBToLinear(u_AlbedoColor); //* v_Color.rgb;

    float metallic = u_MetallicFactor;
    float roughness = u_RoughnessFactor;
    if (u_HasMetallicRoughnessMap != 0) {
        vec4 mrTex = texture(u_MetallicRoughnessMap, v_TexCoord);
        roughness *= mrTex.g;
        metallic *= mrTex.b;
    }

    float ao = u_HasAOMap != 0 ? texture(u_AOMap, v_TexCoord).r : 1.0;

    vec3 N = normalize(v_Normal);
    if (u_HasNormalMap != 0) {
        vec3 normalColor = texture(u_NormalMap, v_TexCoord).rgb * 2.0 - 1.0;
        N = normalize(v_TBN * normalColor);
    }

    // === NORMALS DEBUG ===
    //o_Color = vec4(N * 0.5 + 0.5, 1.0);
    //return;
    // === NORMALS DEBUG ===

    vec3 V = normalize(u_CameraPos - v_FragmentWorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 L = u_LightType == 0 ? normalize(u_LightDir) : normalize(u_LightPos - v_FragmentWorldPos);
    vec3 H = normalize(V + L);

    float attenuation = 1.0;
    float spotEffect = 1.0;
    if (u_LightType != 0) {
        float dist = length(u_LightPos - v_FragmentWorldPos);
        attenuation = 1.0 / (u_LightConstant + u_LightLinear * dist + u_LightQuadratic * (dist * dist));

        if (u_LightType == 2) {
            float cosAlpha = dot(L, normalize(u_LightDir));
            if (cosAlpha > u_LightCutOff) spotEffect = pow(cosAlpha, u_LightExponent);
            else spotEffect = 0.0;
        }
    }
    vec3 radiance = u_LightColor * attenuation * spotEffect;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    // === HALF-LAMBERT ===
    float wrap = dot(N, L) * 0.5 + 0.5;
    float NdotL = wrap * wrap;

    // float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    // === SMART_AMBIENT - FAKE-IBL ===
    // vec3 ambientDiffuse = u_LightAmbient * albedo * kD * ao;

    // === IBL ===
    vec3 irradiance = u_HasIrradianceMap != 0 ? texture(u_IrradianceMap, N).rgb : u_LightAmbient;
    vec3 ambientDiffuse = irradiance * albedo * kD * ao;

    vec3 ambientSpecular = u_LightAmbient * F0 * ao;
    vec3 ambient = ambientDiffuse + ambientSpecular;

    vec3 emission = u_EmissionColor;
    if (u_HasEmissionMap != 0) {
        vec3 texEmi = sRGBToLinear(texture(u_EmissionMap, v_TexCoord).rgb);
        emission = length(u_EmissionColor) > 0.001 ? u_EmissionColor * texEmi : texEmi;
    }

    vec3 color = ambient + Lo + emission;

    color = color / (color + vec3(1.0));
    color = linearToSRGB(color);

    o_Color = vec4(color, albedoTex.a * v_Color.a);
}