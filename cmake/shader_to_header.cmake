# Script to convert a shader file into a C++ header with embedded string literal
# Expects: INPUT_FILE, OUTPUT_FILE, VARIABLE_NAME

file(READ ${INPUT_FILE} SHADER_CONTENT)

# Escape backslashes and quotes
string(REPLACE "\\" "\\\\" SHADER_CONTENT "${SHADER_CONTENT}")
string(REPLACE "\"" "\\\"" SHADER_CONTENT "${SHADER_CONTENT}")

# Split into lines and add quotes
string(REGEX REPLACE "\r?\n" "\\\\n\"\n\"" SHADER_CONTENT "${SHADER_CONTENT}")

# Generate the header file
file(WRITE ${OUTPUT_FILE}
"// Auto-generated shader header
// DO NOT EDIT - Generated from ${INPUT_FILE}

#pragma once

namespace embedded_shaders {

constexpr const char* ${VARIABLE_NAME} =
\"${SHADER_CONTENT}\";

} // namespace embedded_shaders
")
