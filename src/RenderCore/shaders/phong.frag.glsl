#version 450 core

layout (location = 0) out vec4 o_Color;

in vec4 v_Color;
in vec2 v_TexCoord;
in vec3 v_Normal;
in vec3 v_FragmentWorldPos;
in mat3 v_TBN;

uniform vec3 u_MaterialAmbientLighting;
uniform vec3 u_MaterialDiffuseLighting;
uniform vec3 u_MaterialSpecularLighting;
uniform vec3 u_MaterialEmission;
uniform float u_MaterialShininess;

uniform sampler2D u_AlbedoMap;
uniform int u_HasAlbedoMap;

uniform sampler2D u_NormalMap;
uniform int u_HasNormalMap;

uniform sampler2D u_EmissionMap;
uniform int u_HasEmissionMap;

uniform int u_LightType;
uniform vec3 u_LightPos;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform float u_LightConstant;
uniform float u_LightLinear;
uniform float u_LightQuadratic;
uniform float u_LightCutOff;
uniform float u_LightExponent;

uniform vec3 u_CameraPos;

void main()
{
    vec3 surfaceNormal = normalize(v_Normal);
    if (u_HasNormalMap != 0) {
        vec3 normalColor = texture(u_NormalMap, v_TexCoord).rgb;
        normalColor = normalColor * 2.0 - 1.0;
        surfaceNormal = normalize(v_TBN * normalColor);
    }

    vec3 viewDir = normalize(u_CameraPos - v_FragmentWorldPos);
    vec3 lightDir;

    float attenuation = 1.0;
    float spotEffect = 1.0;

    if (u_LightType == 0)
    { // Directional
      lightDir = normalize(u_LightDir);
    }
    else
    { // Point or Spotlight
      lightDir = normalize(u_LightPos - v_FragmentWorldPos);
      float dist = length(u_LightPos - v_FragmentWorldPos);
      attenuation = 1.0 / (u_LightConstant + u_LightLinear * dist + u_LightQuadratic * (dist * dist));

      if (u_LightType == 2)
      { // Spotlight
        float cosAlpha = dot(lightDir, normalize(u_LightDir));
        if (cosAlpha > u_LightCutOff)
        {
            spotEffect = pow(cosAlpha, u_LightExponent);
        }
        else
        {
            spotEffect = 0.0;
        }
      }
    }

    // ALBEDO_MAP
    vec4 surfaceColor = v_Color;
    if (u_HasAlbedoMap != 0) {
        surfaceColor *= texture(u_AlbedoMap, v_TexCoord);
    }

    // Ambient
    vec3 ambientLight = u_MaterialAmbientLighting;

    // Diffuse
    float diffuseIntensity = max(dot(surfaceNormal, lightDir), 0.0);
    vec3 diffuseLight = u_MaterialDiffuseLighting * diffuseIntensity;

    // Specular
    float specFactor = 0.0;
    if (diffuseIntensity > 0.0) {
        vec3 reflectDir = reflect(-lightDir, surfaceNormal);
        specFactor = pow(max(dot(viewDir, reflectDir), 0.0), max(u_MaterialShininess, 1.0));
    }
    vec3 specularLight = u_MaterialSpecularLighting * specFactor;

    vec3 finalAmbient = ambientLight * surfaceColor.rgb;
    vec3 finalDiffuse = diffuseLight * surfaceColor.rgb * attenuation * spotEffect * u_LightColor;
    vec3 finalSpecular = specularLight * attenuation * spotEffect * u_LightColor;

    vec3 finalEmission = u_MaterialEmission;
    if (u_HasEmissionMap != 0) {
        vec3 texEmission = texture(u_EmissionMap, v_TexCoord).rgb;

        if (length(u_MaterialEmission) > 0.001) {
            finalEmission = u_MaterialEmission * texEmission;
        } else {
            finalEmission = texEmission;
        }
    }

    vec3 finalColor = finalEmission + finalAmbient + finalDiffuse + finalSpecular;

    o_Color = vec4(finalColor, surfaceColor.a);
}