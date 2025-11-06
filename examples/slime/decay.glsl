#version 460

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D TrailMap;

layout(location = 5) uniform uint width;
layout(location = 6) uniform uint height;
layout(location = 7) uniform float decayRate;

void main() {
    ivec2 pixelPos = ivec2(gl_GlobalInvocationID.xy);

    // Bounds check
    if (pixelPos.x >= int(width) || pixelPos.y >= int(height)) {
        return;
    }

    // Read current trail value
    vec4 current = imageLoad(TrailMap, pixelPos);

    // Apply simple box blur for diffusion (3x3)
    vec4 sum = current;
    int count = 1;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;

            ivec2 neighborPos = pixelPos + ivec2(dx, dy);

            // Check bounds
            if (neighborPos.x >= 0 && neighborPos.x < int(width) &&
                neighborPos.y >= 0 && neighborPos.y < int(height)) {
                vec4 neighbor = imageLoad(TrailMap, neighborPos);
                sum += neighbor;
                count++;
            }
        }
    }

    // Average with neighbors (diffusion)
    vec4 blurred = sum / float(count);

    // Then apply decay to the blurred result
    vec4 result = blurred * decayRate;

    // Write back
    imageStore(TrailMap, pixelPos, result);
}
