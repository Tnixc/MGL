#version 460

layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

struct Agent {
    vec2 position;
    float angle;
    float padding;
};

layout(std430, binding = 1) buffer AgentBuffer {
    Agent agents[];
};

layout(rgba32f, binding = 0) uniform writeonly image2D TrailMap;

layout(location = 0) uniform uint width;
layout(location = 1) uniform uint height;
layout(location = 2) uniform uint numAgents;
layout(location = 3) uniform float moveSpeed;
layout(location = 4) uniform float deltaTime;

const float PI = 3.14159265359;

uint hash(uint state) {
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    return state;
}

float scaleToRange01(uint random) {
    return float(random) / 4294967295.0;
}

void main() {
    uint id = gl_GlobalInvocationID.x;

    if (id >= numAgents) {
        return;
    }

    Agent agent = agents[id];

    uint random = hash(uint(agent.position.y * float(width) + agent.position.x + hash(id)));

    // Move agent
    vec2 direction = vec2(cos(agent.angle), sin(agent.angle));
    vec2 newPos = agent.position + direction * moveSpeed * deltaTime;

    // Bounce off walls
    if (newPos.x < 0.0 || newPos.x >= float(width) || newPos.y < 0.0 || newPos.y >= float(height)) {
        newPos.x = min(float(width) - 0.01, max(0.0, newPos.x));
        newPos.y = min(float(height) - 0.01, max(0.0, newPos.y));
        agents[id].angle = scaleToRange01(random) * 2.0 * PI;
    }

    // Update position
    agents[id].position = newPos;

    // Draw trail
    ivec2 pixelPos = ivec2(int(newPos.x), int(newPos.y));
    imageStore(TrailMap, pixelPos, vec4(1.0, 1.0, 1.0, 1.0));
}
