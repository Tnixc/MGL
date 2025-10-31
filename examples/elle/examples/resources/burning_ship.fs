#version 460

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

layout(location = 0) uniform float x_min;
layout(location = 1) uniform float x_max;
layout(location = 2) uniform float y_min;
layout(location = 3) uniform float y_max;
layout(location = 4) uniform vec2 z0;
layout(location = 5) uniform int mandelbrot;
layout(location = 6) uniform int max_iter;
layout(location = 7) uniform vec3 colors[5];

void main() {
    vec2 c = vec2(
        x_min + fragTexCoord.x * (x_max - x_min),
        y_min + fragTexCoord.y * (y_max - y_min)
    );

    vec2 z = z0;
    int iter = 0;

    for (int i = 0; i < max_iter; i++) {
        if (mandelbrot != 0) {
            // Mandelbrot
            float x_new = z.x * z.x - z.y * z.y + c.x;
            float y_new = 2.0 * z.x * z.y + c.y;
            z = vec2(x_new, y_new);
        } else {
            // Burning Ship
            float x_new = z.x * z.x - z.y * z.y + c.x;
            float y_new = 2.0 * abs(z.x) * abs(z.y) + c.y;
            z = vec2(x_new, y_new);
        }

        if (dot(z, z) > 4.0) {
            iter = i;
            break;
        }

        if (i == max_iter - 1) {
            iter = max_iter;
        }
    }

    if (iter == max_iter) {
        fragColor = vec4(colors[0], 1.0);
    } else {
        float t = float(iter) / float(max_iter);
        int colorIndex = int(t * 4.0);
        float localT = fract(t * 4.0);

        vec3 color1 = colors[min(colorIndex, 3)];
        vec3 color2 = colors[min(colorIndex + 1, 4)];

        fragColor = vec4(mix(color1, color2, localT), 1.0);
    }
}
