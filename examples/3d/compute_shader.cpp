#include "shader_utils.h"

const char *compute_shader_source = GLSL(
    450 core,

    layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

    layout(binding = 0, rgba32f) writeonly uniform image2D output_image;

    layout(binding = 1) uniform CameraBlock {
        vec3 camera_pos;
        float time;
    };

    // ============================================================================
    // Constants
    // ============================================================================
    const int NUM_SPHERES = 4;
    const int SPHERE_CENTER = 0; const int SPHERE_LEFT = 1; const int SPHERE_RIGHT = 2; const int SPHERE_GROUND = 3;

    const float CENTER_SPHERE_RADIUS = 1.0; const float SIDE_SPHERE_RADIUS = 0.8;
    const float GROUND_SPHERE_RADIUS = 100.0;

    const vec3 CENTER_SPHERE_POS = vec3(0.0, 0.0, -5.0); const vec3 LEFT_SPHERE_POS = vec3(-2.5, 0.0, -5.0);
    const vec3 RIGHT_SPHERE_POS = vec3(2.5, 0.0, -5.0); const vec3 GROUND_SPHERE_POS = vec3(0.0, -101.0, -5.0);

    const vec3 CENTER_SPHERE_COLOR = vec3(1.0, 0.3, 0.3); const vec3 LEFT_SPHERE_COLOR = vec3(0.3, 1.0, 0.3);
    const vec3 RIGHT_SPHERE_COLOR = vec3(0.3, 0.3, 1.0); const vec3 GROUND_COLOR = vec3(0.8, 0.8, 0.8);

    const float BOUNCE_AMPLITUDE = 0.5;

    const vec3 LIGHT_DIRECTION = vec3(0.5, 1.0, 0.3); const float AMBIENT_STRENGTH = 0.3;

    const vec3 SKY_COLOR_TOP = vec3(0.5, 0.7, 1.0); const vec3 SKY_COLOR_BOTTOM = vec3(1.0, 1.0, 1.0);

    const float RAY_T_MAX = 1e10; const float RAY_T_MIN = 0.0;

    const float PI = 3.14159265359; const float TWO_PI = 6.28318530718;

    // ============================================================================
    // Structures
    // ============================================================================
    struct Sphere {
        vec3 center;
        float radius;
        vec3 color;
    };

    struct Ray {
        vec3 origin;
        vec3 direction;
    };

    // ============================================================================
    // Ray-sphere intersection
    // ============================================================================
    bool intersectSphere(Ray ray, Sphere sphere, out float t) {
        vec3 oc = ray.origin - sphere.center;
        float a = dot(ray.direction, ray.direction);
        float b = 2.0 * dot(oc, ray.direction);
        float c = dot(oc, oc) - sphere.radius * sphere.radius;
        float discriminant = b * b - 4.0 * a * c;

        if (discriminant < 0.0)
        {
            return false;
        }

        t = (-b - sqrt(discriminant)) / (2.0 * a);
        return t > RAY_T_MIN;
    }

    // ============================================================================
    // Scene initialization
    // ============================================================================
    void initializeScene(out Sphere spheres[NUM_SPHERES], float time) {
        // Center sphere (animated)
        spheres[SPHERE_CENTER].center = CENTER_SPHERE_POS;
        spheres[SPHERE_CENTER].center.y += sin(time) * BOUNCE_AMPLITUDE;
        spheres[SPHERE_CENTER].radius = CENTER_SPHERE_RADIUS;
        spheres[SPHERE_CENTER].color = CENTER_SPHERE_COLOR;

        // Left sphere
        spheres[SPHERE_LEFT].center = LEFT_SPHERE_POS;
        spheres[SPHERE_LEFT].radius = SIDE_SPHERE_RADIUS;
        spheres[SPHERE_LEFT].color = LEFT_SPHERE_COLOR;

        // Right sphere
        spheres[SPHERE_RIGHT].center = RIGHT_SPHERE_POS;
        spheres[SPHERE_RIGHT].radius = SIDE_SPHERE_RADIUS;
        spheres[SPHERE_RIGHT].color = RIGHT_SPHERE_COLOR;

        // Bottom sphere (ground)
        spheres[SPHERE_GROUND].center = GROUND_SPHERE_POS;
        spheres[SPHERE_GROUND].radius = GROUND_SPHERE_RADIUS;
        spheres[SPHERE_GROUND].color = GROUND_COLOR;
    }

    // ============================================================================
    // Lighting calculations
    // ============================================================================
    vec3 calculateLighting(vec3 hit_point, vec3 sphere_center, vec3 sphere_color) {
        vec3 normal = normalize(hit_point - sphere_center);

        vec3 light_dir = normalize(LIGHT_DIRECTION);
        float diffuse = max(dot(normal, light_dir), 0.0);

        vec3 ambient = sphere_color * AMBIENT_STRENGTH;
        vec3 lit = sphere_color * diffuse;

        return ambient + lit;
    }

    vec3 getSkyColor(vec3 ray_direction) {
        float gradient = ray_direction.y * 0.5 + 0.5;
        return mix(SKY_COLOR_BOTTOM, SKY_COLOR_TOP, gradient);
    }

    // ============================================================================
    // Main ray tracing function
    // ============================================================================
    vec3 trace(Ray ray, float time) {
        Sphere spheres[NUM_SPHERES];
        initializeScene(spheres, time);

        float closest_t = RAY_T_MAX;
        int hit_sphere = -1;
        float t;

        for (int i = 0; i < NUM_SPHERES; i++)
        {
            if (intersectSphere(ray, spheres[i], t))
            {
                if (t < closest_t)
                {
                    closest_t = t;
                    hit_sphere = i;
                }
            }
        }

        if (hit_sphere == -1)
        {
            return getSkyColor(ray.direction);
        }

        vec3 hit_point = ray.origin + ray.direction * closest_t;
        return calculateLighting(hit_point, spheres[hit_sphere].center, spheres[hit_sphere].color);
    }

    // ============================================================================
    // Compute shader main
    // ============================================================================
    void main() {
        ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
        ivec2 size = imageSize(output_image);

        if (coord.x >= size.x || coord.y >= size.y)
            return;

        // Calculate normalized device coordinates
        vec2 uv = (vec2(coord) + 0.5) / vec2(size);
        uv = uv * 2.0 - 1.0;
        uv.x *= float(size.x) / float(size.y);

        // Setup camera ray
        Ray ray;
        ray.origin = camera_pos;
        ray.direction = normalize(vec3(uv.x, uv.y, -1.0));

        // Trace ray
        vec3 color = trace(ray, time);

        imageStore(output_image, coord, vec4(color, 1.0));
    });
