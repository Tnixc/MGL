#version 460

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D screenTexture;

void main()
{
    // Sample the texture
    FragColor = texture(screenTexture, TexCoord);
}
