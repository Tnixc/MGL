cmake_minimum_required(VERSION 3.5)

project(glfw VERSION 3.3.8)

# Build only Cocoa backend for macOS
set(GLFW_BUILD_COCOA ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_WIN32 OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)

# Disable unnecessary components
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Find required frameworks
find_library(METAL_FRAMEWORK Metal REQUIRED)
find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
find_library(COREGRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
find_library(CORESERVICES_FRAMEWORK CoreServices REQUIRED)

# Set up source files
set(GLFW_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(GLFW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main library sources
set(GLFW_SOURCES
    ${GLFW_SOURCE_DIR}/context.c
    ${GLFW_SOURCE_DIR}/init.c
    ${GLFW_SOURCE_DIR}/input.c
    ${GLFW_SOURCE_DIR}/monitor.c
    ${GLFW_SOURCE_DIR}/platform.c
    ${GLFW_SOURCE_DIR}/vulkan.c
    ${GLFW_SOURCE_DIR}/window.c
    ${GLFW_SOURCE_DIR}/egl_context.c
    ${GLFW_SOURCE_DIR}/osmesa_context.c
    ${GLFW_SOURCE_DIR}/null_platform.h
    ${GLFW_SOURCE_DIR}/null_joystick.h
    ${GLFW_SOURCE_DIR}/null_init.c
    ${GLFW_SOURCE_DIR}/null_monitor.c
    ${GLFW_SOURCE_DIR}/null_window.c
    ${GLFW_SOURCE_DIR}/null_joystick.c
)

# macOS/Cocoa specific sources
if (APPLE)
    list(APPEND GLFW_SOURCES
        ${GLFW_SOURCE_DIR}/cocoa_time.h
        ${GLFW_SOURCE_DIR}/cocoa_time.c
        ${GLFW_SOURCE_DIR}/posix_thread.h
        ${GLFW_SOURCE_DIR}/posix_module.c
        ${GLFW_SOURCE_DIR}/posix_thread.c
        ${GLFW_SOURCE_DIR}/cocoa_platform.h
        ${GLFW_SOURCE_DIR}/cocoa_joystick.h
        ${GLFW_SOURCE_DIR}/cocoa_init.m
        ${GLFW_SOURCE_DIR}/cocoa_joystick.m
        ${GLFW_SOURCE_DIR}/cocoa_monitor.m
        ${GLFW_SOURCE_DIR}/cocoa_window.m
        ${GLFW_SOURCE_DIR}/mgl_context.m  # Our custom MGL context
    )
endif()

# Create the library
add_library(glfw SHARED ${GLFW_SOURCES})

# Include directories
target_include_directories(glfw PUBLIC ${GLFW_INCLUDE_DIR})
target_include_directories(glfw PRIVATE ../include)  # For MGL headers

# Compile definitions
target_compile_definitions(glfw PRIVATE _GLFW_COCOA)

# Link libraries
target_link_libraries(glfw PRIVATE
    mgl
    ${METAL_FRAMEWORK}
    ${FOUNDATION_FRAMEWORK}
    ${APPKIT_FRAMEWORK}
    ${IOKIT_FRAMEWORK}
    ${COREFOUNDATION_FRAMEWORK}
    ${COREGRAPHICS_FRAMEWORK}
    ${CORESERVICES_FRAMEWORK}
)

# Set properties
set_target_properties(glfw PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Install target (optional, local only)
install(TARGETS glfw
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}
    ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}
)
