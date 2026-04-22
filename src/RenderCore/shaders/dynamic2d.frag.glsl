#version 450 core

layout (location = 0) out vec4 color;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;

uniform sampler2D u_Texture;

void main()
{
    if (v_TexIndex < 0.0)
    {
        color = v_Color;
    }
    else
    {
        color = texture(u_Texture, v_TexCoord) * v_Color;
    }
}