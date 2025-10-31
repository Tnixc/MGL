# Function to embed shader files as C++ string literals
# Usage: embed_shaders(TARGET target_name SHADERS shader1.vert shader2.frag ...)
function(embed_shaders)
    cmake_parse_arguments(EMBED_SHADERS "" "TARGET" "SHADERS" ${ARGN})

    set(GENERATED_HEADERS "")

    foreach(SHADER_FILE ${EMBED_SHADERS_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
        string(REPLACE "." "_" SHADER_VAR_NAME ${SHADER_NAME})

        set(OUTPUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/${SHADER_NAME}.h")

        add_custom_command(
            OUTPUT ${OUTPUT_HEADER}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
            COMMAND ${CMAKE_COMMAND}
                -DINPUT_FILE=${SHADER_FILE}
                -DOUTPUT_FILE=${OUTPUT_HEADER}
                -DVARIABLE_NAME=${SHADER_VAR_NAME}
                -P ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/shader_to_header.cmake
            DEPENDS ${SHADER_FILE}
            COMMENT "Embedding shader ${SHADER_NAME}"
            VERBATIM
        )

        list(APPEND GENERATED_HEADERS ${OUTPUT_HEADER})
    endforeach()

    # Add the generated headers to the target's sources
    target_sources(${EMBED_SHADERS_TARGET} PRIVATE ${GENERATED_HEADERS})

    # Add the generated directory to include path
    target_include_directories(${EMBED_SHADERS_TARGET} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
endfunction()
