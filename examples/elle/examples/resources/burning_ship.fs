#version 460

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    // Simple test: output blue to verify shader is working
    fragColor = vec4(0.0, 0.0, 1.0, 1.0);
}
