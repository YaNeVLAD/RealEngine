#version 450 core

layout (location = 0) out vec4 o_Color;

in vec3 v_TexCoord;

uniform samplerCube u_EnvironmentMap;

vec3 linearToSRGB(vec3 linearIn) { return pow(linearIn, vec3(1.0 / 2.2)); }

void main()
{
    vec3 envColor = texture(u_EnvironmentMap, v_TexCoord).rgb;

    // Tone mapping
    envColor = envColor / (envColor + vec3(1.0));
    // Gamma
    envColor = linearToSRGB(envColor);

    o_Color = vec4(envColor, 1.0);
}