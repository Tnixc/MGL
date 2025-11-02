#version 430

layout(location = 0) in vec2 fragTexCoord;

layout(location = 2) uniform float time;

layout(location = 0) out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;
    uv -= vec2(0.5, 0.5);

    float angle = time * 0.3 * 3.14159;
    mat2 rotationMatrix = mat2(cos(angle), -sin(angle),
                                sin(angle), cos(angle));

    uv *= rotationMatrix;
    uv += vec2(0.5, 0.5);
    finalColor = vec4(uv, 0.0, 1.0);
}

