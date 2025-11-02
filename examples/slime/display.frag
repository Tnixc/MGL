#version 460

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;

uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoord);
}
