#version 460

// Input vertex attributes (from vertex shader)
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

// Output fragment color
layout(location = 0) out vec4 finalColor;

void main() {
    // Output bright magenta to make shader artifacts obvious
    finalColor = vec4(1.0, 0.0, 1.0, 1.0);
}