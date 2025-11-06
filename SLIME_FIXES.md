# Slime Compute Shader Fixes

## Summary
Fixed critical compute shader pipeline issues in the slime mold simulation that prevented proper trail rendering.

## Problems Identified

### 1. **Texture Access Mode Issue** ❌
**File:** `compute.glsl:15`
```glsl
// BEFORE (WRONG)
layout(rgba32f, binding = 0) uniform writeonly image2D TrailMap;
```

**Problem:** Texture was `writeonly`, preventing the shader from reading existing trail values for accumulation.

### 2. **No Trail Accumulation** ❌
**File:** `compute.glsl:67`
```glsl
// BEFORE (WRONG)
imageStore(TrailMap, pixelPos, vec4(1.0, 1.0, 1.0, 1.0));
```

**Problem:** Directly overwrote pixels with white (1.0) instead of:
- Reading existing values
- Adding to them (deposition)
- Clamping to prevent overflow

**Result:** Only current agent positions visible, no persistent trails.

### 3. **Missing Decay/Diffusion** ❌
**Problem:** No mechanism to fade trails over time or blur them for smooth visuals.

**Result:** Trails would accumulate to solid white and never fade, making the simulation unusable.

### 4. **Wrong Texture Binding Mode** ❌
**File:** `slime.cpp:315`
```cpp
// BEFORE (WRONG)
glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
```

**Problem:** C++ side was binding texture as `GL_WRITE_ONLY`, inconsistent with shader needs.

## Fixes Applied

### Fix 1: Enable Read-Write Texture Access ✅
**File:** `compute.glsl:15-16`
```glsl
// AFTER (CORRECT)
// Changed from writeonly to allow reading AND writing for trail accumulation
layout(rgba32f, binding = 0) uniform image2D TrailMap;
```

### Fix 2: Implement Trail Accumulation ✅
**File:** `compute.glsl:65-81`
```glsl
// AFTER (CORRECT)
// Draw trail by ADDING to existing trail value (accumulation)
ivec2 pixelPos = ivec2(int(newPos.x), int(float(height) - 1.0 - newPos.y));

// Check bounds before accessing texture
if (pixelPos.x >= 0 && pixelPos.x < int(width) && pixelPos.y >= 0 && pixelPos.y < int(height)) {
    // Read existing trail value
    vec4 currentTrail = imageLoad(TrailMap, pixelPos);

    // Add agent deposit to trail (0.1 per agent)
    vec4 newTrail = currentTrail + vec4(0.1, 0.1, 0.1, 0.0);

    // Clamp to prevent overflow
    newTrail = min(newTrail, vec4(1.0));

    imageStore(TrailMap, pixelPos, newTrail);
}
```

### Fix 3: Add Decay and Diffusion Shader ✅
**File:** `decay.glsl` (NEW)

Created separate compute shader for trail processing:
- **Diffusion:** 3x3 box blur averaging with neighbors
- **Decay:** Multiplies trail intensity by decay rate (0.95 = 5% fade per frame)
- **Bounds checking:** Safe neighbor access

**Key features:**
```glsl
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D TrailMap;

// Simple diffusion: average with neighbors (3x3 box blur)
// Apply decay: trail * decayRate
```

### Fix 4: Update C++ Pipeline ✅
**File:** `slime.cpp`

**Added decay program creation:**
```cpp
GLuint createDecayProgram() { /* loads and compiles decay.glsl */ }
```

**Changed texture binding mode:**
```cpp
// Changed from GL_WRITE_ONLY to GL_READ_WRITE
glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
```

**Implemented two-pass compute pipeline:**
```cpp
// Pass 1: Agent movement and trail deposition
glUseProgram(computeProgram);
glDispatchCompute((NUM_AGENTS + 15) / 16, 1, 1);
glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

// Pass 2: Trail decay and diffusion
glUseProgram(decayProgram);
glUniform1ui(5, width);
glUniform1ui(6, height);
glUniform1f(7, DECAY_RATE);
glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
```

**Added cleanup:**
```cpp
glDeleteProgram(decayProgram);
```

## How the Fixed Pipeline Works

1. **Agent Pass** (compute.glsl)
   - Each agent moves based on its angle
   - Bounces off walls with random new direction
   - **Reads** existing trail value at its position
   - **Adds** 0.1 to the trail (pheromone deposition)
   - **Clamps** result to max 1.0

2. **Decay Pass** (decay.glsl)
   - Runs on every pixel (16x16 work groups)
   - **Diffuses** trails by averaging with 8 neighbors (3x3 blur)
   - **Decays** trails by multiplying by 0.95 (5% fade per frame)
   - Creates smooth, organic-looking trail patterns

3. **Display Pass** (display shaders)
   - Renders full-screen quad with trail texture
   - Shows accumulated and decayed trails

## Expected Behavior After Fixes

✅ Agents leave visible trails as they move
✅ Trails persist across frames (accumulation)
✅ Trails fade slowly over time (decay)
✅ Trails blur for smooth visual effect (diffusion)
✅ No overflow (trails clamped to [0, 1])
✅ Proper slime mold emergence patterns

## Comparison: Working vs Non-Working

### Working Example (compute_shader_example.cpp)
- ✅ Uses simple writeonly texture (no accumulation needed)
- ✅ Generates patterns procedurally each frame
- ✅ No persistence required
- ✅ Works correctly

### Fixed Slime Example
- ✅ Now uses read-write texture for accumulation
- ✅ Properly persists trails across frames
- ✅ Decay prevents white-out
- ✅ Diffusion creates organic patterns

## Technical Details

**Texture Format:** `GL_RGBA32F` (32-bit float per channel)
- Allows fine-grained accumulation (0.1 increments)
- Prevents banding artifacts
- Supports values [0.0, 1.0]

**Work Group Sizes:**
- Agent pass: 16x1x1 (one thread per agent)
- Decay pass: 16x16x1 (covers all pixels efficiently)

**Memory Barriers:**
- `GL_SHADER_IMAGE_ACCESS_BARRIER_BIT`: Ensures texture writes complete
- `GL_SHADER_STORAGE_BARRIER_BIT`: Ensures SSBO writes complete
- Critical for correct read-after-write behavior

## Files Modified

1. ✏️ `examples/slime/compute.glsl` - Fixed texture access and trail accumulation
2. ✏️ `examples/slime/slime.cpp` - Updated pipeline and texture binding
3. ➕ `examples/slime/decay.glsl` - NEW decay/diffusion shader

## Testing Recommendations

1. Build and run: `./build/examples/slime`
2. Verify agents spawn at center
3. Confirm trails appear and persist
4. Observe trails fading over ~20 frames (95% decay rate)
5. Check for smooth blur effect (diffusion)
6. Adjust parameters:
   - `DECAY_RATE` (0.90-0.99): faster/slower fade
   - Agent deposit amount (0.05-0.2): brighter/dimmer trails
   - `MOVE_SPEED`: faster/slower movement

## Root Cause Analysis

The original code was structured like a **stateless** compute shader (like the working example), but the slime simulation requires **stateful** behavior with trail persistence. The key insight is:

- **Stateless shaders**: Generate output purely from inputs (working example)
- **Stateful shaders**: Read previous output, modify, write back (slime simulation)

The fixes properly converted the slime shader from stateless to stateful by enabling read-write texture access and implementing proper accumulation logic.
