#version 460

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 finalColor;

void main() {
    // Simple test: output UV coordinates as colors
    // This should show red->yellow gradient horizontally, and add green vertically
    finalColor = vec4(fragTexCoord.x, fragTexCoord.y, 0.0, 1.0);
}
