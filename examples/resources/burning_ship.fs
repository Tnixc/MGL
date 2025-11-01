#version 460

// Input vertex attributes (from vertex shader)
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

// Output fragment color
layout(location = 0) out vec4 finalColor;

void main() {
    // Display UV coordinates as colors (red = U, green = V, blue = 1.0, alpha = 1.0)
    finalColor = vec4(fragTexCoord, 1.0, 1.0);
}