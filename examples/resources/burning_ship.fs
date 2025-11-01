#version 460

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 finalColor;

void main() {
    finalColor = vec4(fragTexCoord, 1.0, 1.0);
}
