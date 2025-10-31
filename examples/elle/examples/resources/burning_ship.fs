#version 460

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    // Test shader: color based on texture coordinates
    // Top-left should be black, bottom-right should be yellow
    fragColor = vec4(fragTexCoord.x, fragTexCoord.y, 0.0, 1.0);
}
