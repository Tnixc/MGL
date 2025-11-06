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

// Uniform buffer for compute parameters
layout(std140, binding = 2) uniform ComputeParams {
    uint width;
    uint height;
    uint numAgents;
    float moveSpeed;
    float deltaTime;
};

// Read-write access for trail accumulation
layout(rgba32f, binding = 0) uniform image2D TrailMap;

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
    
    // Load agent data
    Agent agent = agents[id];
    
    // Calculate movement direction from angle
    vec2 direction = vec2(cos(agent.angle), sin(agent.angle));
    
    // Calculate new position
    vec2 newPos = agent.position + direction * moveSpeed * deltaTime;
    
    // Handle boundary collisions with reflection
    if (newPos.x < 0.0) {
        newPos.x = 0.0;
        agent.angle = PI - agent.angle; // Reflect angle horizontally
    }
    else if (newPos.x >= float(width)) {
        newPos.x = float(width) - 1.0;
        agent.angle = PI - agent.angle; // Reflect angle horizontally
    }
    
    if (newPos.y < 0.0) {
        newPos.y = 0.0;
        agent.angle = -agent.angle; // Reflect angle vertically
    }
    else if (newPos.y >= float(height)) {
        newPos.y = float(height) - 1.0;
        agent.angle = -agent.angle; // Reflect angle vertically
    }
    
    // Normalize angle to [0, 2π]
    while (agent.angle < 0.0) agent.angle += 2.0 * PI;
    while (agent.angle >= 2.0 * PI) agent.angle -= 2.0 * PI;
    
    // Update agent in buffer
    agents[id].position = newPos;
    agents[id].angle = agent.angle;
    
    // Deposit trail at agent position
    ivec2 pixelPos = ivec2(newPos);
    
    // Bounds check before writing to texture
    if (pixelPos.x >= 0 && pixelPos.x < int(width) && 
        pixelPos.y >= 0 && pixelPos.y < int(height)) {
        
        // Read current trail value
        vec4 currentTrail = imageLoad(TrailMap, pixelPos);
        
        // Add deposit (brighter trails)
        float depositAmount = 0.5;
        vec4 newTrail = currentTrail + vec4(depositAmount, depositAmount, depositAmount, 0.0);
        
        // Clamp to prevent overflow
        newTrail = clamp(newTrail, vec4(0.0), vec4(1.0));
        newTrail.a = 1.0; // Keep alpha at 1.0
        
        // Write back to trail map
        imageStore(TrailMap, pixelPos, newTrail);
    }
    
    // DEBUG: On first frame, mark the center with a cross
    if (deltaTime < 0.02 && id < 10) { // First frame for first 10 agents
        ivec2 centerPos = ivec2(int(width) / 2, int(height) / 2);
        
        // Draw a small cross at center for reference
        for (int i = -3; i <= 3; i++) {
            ivec2 hPos = centerPos + ivec2(i, 0);
            ivec2 vPos = centerPos + ivec2(0, i);
            
            if (hPos.x >= 0 && hPos.x < int(width) && hPos.y >= 0 && hPos.y < int(height)) {
                imageStore(TrailMap, hPos, vec4(1.0, 0.0, 0.0, 1.0)); // Red horizontal
            }
            if (vPos.x >= 0 && vPos.x < int(width) && vPos.y >= 0 && vPos.y < int(height)) {
                imageStore(TrailMap, vPos, vec4(0.0, 1.0, 0.0, 1.0)); // Green vertical
            }
        }
    }
}
