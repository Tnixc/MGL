#version 460

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D outputTexture;

layout(location = 0) uniform uint width;
layout(location = 1) uniform uint height;

// Hash function from www.cs.ubc.ca/~rbridson/docs/schechter-sca08-turbulence.pdf
uint hash(uint state) {
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    return state;
}

void main() {
    uvec2 id = gl_GlobalInvocationID.xy;

    // Exit if the current thread is outside the texture bounds
    if (id.x >= width || id.y >= height) {
        return;
    }

    // Calculate pixel index
    int pixelIndex = int(id.y * width + id.x);

    // Generate pseudo-random number based on pixel position
    uint pseudoRandomNumber = hash(uint(pixelIndex));

    // Normalize to [0, 1] and output as color
    float value = float(pseudoRandomNumber) / 4294967295.0;

    // Create different patterns for R, G, B channels
    float r = value;
    float g = float(hash(pseudoRandomNumber)) / 4294967295.0;
    float b = float(hash(pseudoRandomNumber * 2u)) / 4294967295.0;

    imageStore(outputTexture, ivec2(id), vec4(r, g, b, 1.0));
}
