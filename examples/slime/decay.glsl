#version 460

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D TrailMap;

layout(std140, binding = 2) uniform DecayParams {
    uint width;
    uint height;
    float decayRate;
};

void main() {
    ivec2 pixelPos = ivec2(gl_GlobalInvocationID.xy);

    // Bounds check
    if (pixelPos.x >= int(width) || pixelPos.y >= int(height)) {
        return;
    }

    // Read current trail value
    vec4 current = imageLoad(TrailMap, pixelPos);
    
    // Simple decay - just multiply by decay rate
    vec4 decayed = current * decayRate;
    
    // Ensure alpha stays at 1.0
    decayed.a = 1.0;
    
    // Write back
    imageStore(TrailMap, pixelPos, decayed);
}
