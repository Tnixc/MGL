cmake_minimum_required(VERSION 4.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include shader embedding functionality
include(${CMAKE_SOURCE_DIR}/cmake/embed_shaders.cmake)

set(EXAMPLE_COMPILE_DEFS
    ENABLE_OPT=0
    SPIRV_CROSS_C_API_MSL=1
    SPIRV_CROSS_C_API_GLSL=1
    SPIRV_CROSS_C_API_CPP=1
    SPIRV_CROSS_C_API_REFLECT=1
)

add_executable(mandelbrot_compute mandelbrot_compute.cpp)
target_include_directories(mandelbrot_compute PUBLIC ${SDL2_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/glfw/include)
target_link_libraries(mandelbrot_compute glfw mgl)
target_compile_definitions(mandelbrot_compute PUBLIC ${EXAMPLE_COMPILE_DEFS})

add_executable(test_compute_minimal test_compute_minimal.cpp)
target_include_directories(test_compute_minimal PUBLIC ${SDL2_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/glfw/include)
target_link_libraries(test_compute_minimal glfw mgl)
target_compile_definitions(test_compute_minimal PUBLIC ${EXAMPLE_COMPILE_DEFS})

add_executable(compute_shader_example compute_shader_example.cpp)
target_include_directories(compute_shader_example PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(compute_shader_example mgl)
target_compile_definitions(compute_shader_example PUBLIC ${EXAMPLE_COMPILE_DEFS})
target_compile_options(compute_shader_example PUBLIC -fsanitize=undefined,address)
target_link_options(compute_shader_example PUBLIC -fsanitize=undefined,address)

add_executable(conway conway.cpp)
target_include_directories(conway PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(conway mgl)
target_compile_definitions(conway PUBLIC ${EXAMPLE_COMPILE_DEFS})
target_compile_options(conway PUBLIC -fsanitize=undefined,address)
target_link_options(conway PUBLIC -fsanitize=undefined,address)

add_executable(3d
    3d/main.cpp
    3d/ray_tracer.cpp
    3d/shader_utils.cpp
    3d/compute_shader.cpp
    3d/vertex_shader.cpp
    3d/fragment_shader.cpp
)
target_include_directories(3d PUBLIC ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/3d)
target_link_libraries(3d mgl)
target_compile_definitions(3d PUBLIC ${EXAMPLE_COMPILE_DEFS})
target_compile_options(3d PUBLIC -fsanitize=undefined,address)
target_link_options(3d PUBLIC -fsanitize=undefined,address)

add_executable(hello_triangle hello_triangle/hello_colorful_triangle.cpp)
target_include_directories(hello_triangle PUBLIC ${SDL2_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/glfw/include)
target_link_libraries(hello_triangle glfw mgl)
target_compile_definitions(hello_triangle PUBLIC ${EXAMPLE_COMPILE_DEFS})
target_compile_options(hello_triangle PUBLIC -fsanitize=undefined,address)
target_link_options(hello_triangle PUBLIC -fsanitize=undefined,address)

# embed shader files at compile time
# NOTE: idk if this is the right way to do it, doesn't seem like it
embed_shaders(
    TARGET hello_triangle
    SHADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/hello_triangle/shader_triangle.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/hello_triangle/shader_triangle.frag
)
