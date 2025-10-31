#include "shader_utils.h"

const char *fragment_shader_source = GLSL(
    450 core,

    layout(location = 0) in vec2 v_texcoord;
    layout(location = 0) out vec4 frag_color;

    layout(binding = 0) uniform sampler2D tex;

    void main() { frag_color = texture(tex, v_texcoord); });
