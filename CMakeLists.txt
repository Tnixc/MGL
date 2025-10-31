cmake_minimum_required(VERSION 3.5)
project(mgl C CXX OBJC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_subdirectory(subprojects)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glslang REQUIRED)
find_package(SPIRV-Tools REQUIRED)
find_package(SDL2 REQUIRED)

find_library(METAL_FRAMEWORK Metal)
find_library(OPENGL_FRAMEWORK OpenGL)
find_library(FOUNDATION_FRAMEWORK Foundation)
find_library(QUARTZCORE_FRAMEWORK QuartzCore)
find_library(APPKIT_FRAMEWORK AppKit)

file(GLOB c_srcs src/*.c)
file(GLOB objc_srcs src/*.m)
set_source_files_properties(${objc_srcs} PROPERTIES COMPILE_FLAGS "-fobjc-arc")

add_library(mgl SHARED
    ${c_srcs}
    ${objc_srcs})

target_compile_options(mgl PUBLIC -fsanitize=undefined,address)
target_link_options(mgl PUBLIC -fsanitize=undefined,address)

target_compile_definitions(mgl PUBLIC 
ENABLE_OPT=0 
DEBUG_LEVEL=1
DEBUG=1
SPIRV_CROSS_C_API_MSL=1
SPIRV_CROSS_C_API_GLSL=1
SPIRV_CROSS_C_API_CPP=1
SPIRV_CROSS_C_API_REFLECT=1
                                )

target_include_directories(mgl PUBLIC include/ include/GL ${SDL2_INCLUDE_DIRS})

target_link_libraries(mgl
    glslang::glslang
    glslang::SPIRV
    glslang::glslang-default-resource-limits
    spirv-cross-core
    spirv-cross-c
    spirv-cross-cpp
    spirv-cross-msl
    spirv-cross-glsl
    spirv-cross-hlsl
    spirv-cross-reflect
    SDL2::SDL2
    ${METAL_FRAMEWORK}
    ${FOUNDATION_FRAMEWORK}
    ${OPENGL_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
    ${APPKIT_FRAMEWORK}
)

add_subdirectory(glfw)
add_subdirectory(examples)

find_package(SDL2 REQUIRED)

# Google Test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(mgl_test test/gtest_main.cpp)
# NOTE: Your glm would be here if you used homebrew. I don't think this is a good idea though
target_include_directories(mgl_test PUBLIC ${SDL2_INCLUDE_DIRS} /opt/homebrew/include)
target_link_libraries(mgl_test mgl SDL2::SDL2 gtest_main)

target_compile_definitions(mgl_test PUBLIC
                                ENABLE_OPT=0
                                SPIRV_CROSS_C_API_MSL=1
                                SPIRV_CROSS_C_API_GLSL=1
                                SPIRV_CROSS_C_API_CPP=1
                                SPIRV_CROSS_C_API_REFLECT=1
                                )

target_compile_options(mgl_test PUBLIC -fsanitize=undefined,address)
target_link_options(mgl_test PUBLIC -fsanitize=undefined,address)

# important, i think
install(TARGETS mgl
    LIBRARY DESTINATION /usr/local/lib
)


