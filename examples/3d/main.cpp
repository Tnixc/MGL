#include "ray_tracer.h"
#include <print>

int main(int argc, char **argv)
{
    RayTracer tracer;

    if (!tracer.initialize())
    {
        std::print(stderr, "Failed to initialize ray tracer\n");
        return 1;
    }

    tracer.run();

    return 0;
}
