/*
 * Copyright (C) Michael Larson on 1/6/2022
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * program.c
 * MGL
 *
 */

#include <stdio.h>
#include <string.h>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Include/glslang_c_shader_types.h>
#include "spirv-tools/libspirv.h"
#include "spirv_cross_c.h"
#include "spirv.h"

#include "glm_context.h"
#include "shaders.h"
#include "buffers.h"

Program *newProgram(GLMContext ctx, GLuint program)
{
    Program *ptr;

    ptr = (Program *)malloc(sizeof(Program));
    assert(ptr);

    bzero(ptr, sizeof(Program));

    ptr->name = program;

    return ptr;
}

Program *getProgram(GLMContext ctx, GLuint program)
{
    Program *ptr;

    ptr = (Program *)searchHashTable(&STATE(program_table), program);

    if (!ptr)
    {
        ptr = newProgram(ctx, program);

        insertHashElement(&STATE(program_table), program, ptr);
    }

    return ptr;
}

int isProgram(GLMContext ctx, GLuint program)
{
    Program *ptr;

    ptr = (Program *)searchHashTable(&STATE(program_table), program);

    if (ptr)
        return 1;

    return 0;
}

Program *findProgram(GLMContext ctx, GLuint program)
{
    Program *ptr;

    ptr = (Program *)searchHashTable(&STATE(program_table), program);

    return ptr;
}

GLuint mglCreateProgram(GLMContext ctx)
{
    GLuint program;

    program = getNewName(&STATE(program_table));

    getProgram(ctx, program);

    return program;
}

void mglDeleteProgram(GLMContext ctx, GLuint program)
{
    Program *ptr;

    ptr = findProgram(ctx, program);

    if (!ptr)
    {
        assert(0);

        return;
    }

    deleteHashElement(&STATE(program_table), program);

    if (ptr->linked_glsl_program)
    {
        glslang_program_delete(ptr->linked_glsl_program);
    }

    // Free mtl_data entry points
    for (int i = 0; i < _MAX_SHADER_TYPES; i++)
    {
        if (ptr->mtl_data[i].entry_point)
        {
            free((void *)ptr->mtl_data[i].entry_point);
        }
    }

    // Free SPIRV data
    for (int i = 0; i < _MAX_SHADER_TYPES; i++)
    {
        if (ptr->spirv[i].ir)
        {
            free(ptr->spirv[i].ir);
        }
        if (ptr->spirv[i].msl_str)
        {
            free(ptr->spirv[i].msl_str);
        }
    }

    // Clean up attached shaders that are marked for deletion
    for (int i = 0; i < _MAX_SHADER_TYPES; i++)
    {
        Shader *shader = ptr->shader_slots[i];
        if (shader && shader->delete_pending)
        {
            // Clean up the shader resources
            if (shader->compiled_glsl_shader)
            {
                glslang_shader_delete(shader->compiled_glsl_shader);
            }
            if (shader->mtl_data.library)
            {
                ctx->mtl_funcs.mtlDeleteMTLObj(ctx, shader->mtl_data.function);
                ctx->mtl_funcs.mtlDeleteMTLObj(ctx, shader->mtl_data.library);
            }
            if (shader->log)
            {
                free(shader->log);
            }
            free((void *)shader->mtl_shader_type_name);
            free((void *)shader->src);
            free(shader);
        }
    }

    free(ptr);
}

GLboolean mglIsProgram(GLMContext ctx, GLuint program)
{
    if (isProgram(ctx, program))
        return GL_TRUE;

    return GL_FALSE;
}

void mglAttachShader(GLMContext ctx, GLuint program, GLuint shader)
{
    Program *pptr;
    Shader *sptr;
    GLuint index;

    sptr = findShader(ctx, shader);

    if (!sptr)
    {
        assert(0);

        return;
    }

    pptr = findProgram(ctx, program);

    if (!pptr)
    {
        assert(0);

        return;
    }

    index = sptr->glm_type;

    pptr->shader_slots[index] = sptr;
    pptr->dirty_bits |= DIRTY_PROGRAM;
}

void mglDetachShader(GLMContext ctx, GLuint program, GLuint shader)
{
    Program *pptr;
    Shader *sptr;
    GLuint index;

    // Note: findShader won't work if the shader was already deleted
    // We need to search the program's shader slots instead
    pptr = findProgram(ctx, program);

    if (!pptr)
    {
        assert(0);

        return;
    }

    // Find the shader in the program's shader slots
    sptr = NULL;
    index = 0;
    for (int i = 0; i < _MAX_SHADER_TYPES; i++)
    {
        if (pptr->shader_slots[i] && pptr->shader_slots[i]->name == shader)
        {
            sptr = pptr->shader_slots[i];
            index = i;
            break;
        }
    }

    if (!sptr)
    {
        ERROR_CHECK_RETURN(false, GL_INVALID_VALUE);
        return;
    }

    pptr->shader_slots[index] = NULL;
    pptr->dirty_bits |= DIRTY_PROGRAM;

    // If the shader is marked for deletion, clean it up now
    if (sptr->delete_pending)
    {
        if (sptr->compiled_glsl_shader)
        {
            glslang_shader_delete(sptr->compiled_glsl_shader);
        }
        if (sptr->mtl_data.library)
        {
            ctx->mtl_funcs.mtlDeleteMTLObj(ctx, sptr->mtl_data.function);
            ctx->mtl_funcs.mtlDeleteMTLObj(ctx, sptr->mtl_data.library);
        }
        if (sptr->log)
        {
            free(sptr->log);
        }
        free((void *)sptr->mtl_shader_type_name);
        free((void *)sptr->src);
        free(sptr);
    }
}

void error_callback(void *userdata, const char *error)
{
    assert(error);
    DEBUG_PRINT("parseSPIRVShader error:%s\n", error);
}

static_assert(_VERTEX_SHADER == GLSLANG_STAGE_VERTEX, "_VERTEX_SHADER == GLSLANG_STAGE_VERTEX failed");
static_assert(_TESS_CONTROL_SHADER == GLSLANG_STAGE_TESSCONTROL,
              "_TESS_CONTROL_SHADER == GLSLANG_STAGE_TESSCONTROL failed");
static_assert(_TESS_EVALUATION_SHADER == GLSLANG_STAGE_TESSEVALUATION,
              "_TESS_EVALUATION_SHADER == GLSLANG_STAGE_TESSEVALUATION failed");
static_assert(_GEOMETRY_SHADER == GLSLANG_STAGE_GEOMETRY, "_GEOMETRY_SHADER == GLSLANG_STAGE_GEOMETRY failed");
static_assert(_FRAGMENT_SHADER == GLSLANG_STAGE_FRAGMENT, "_FRAGMENT_SHADER == GLSLANG_STAGE_FRAGMENT failed");
static_assert(_COMPUTE_SHADER == GLSLANG_STAGE_COMPUTE, "_COMPUTE_SHADER == GLSLANG_STAGE_COMPUTE failed");

void addShadersToProgram(GLMContext ctx, Program *pptr, glslang_program_t *glsl_program)
{
    // add shaders
    for (int i = 0; i < _MAX_SHADER_TYPES; i++)
    {
        Shader *ptr;

        ptr = pptr->shader_slots[i];

        if (ptr)
        {
            // should have glsl shader here
            if (!ptr->compiled_glsl_shader)
            {
                DEBUG_PRINT("Shader stage %d has no compiled GLSL shader\n", i);
                ERROR_CHECK_RETURN(false, GL_INVALID_OPERATION);
            }

            DEBUG_PRINT("Adding shader stage %d (type %d) to program, glsl_shader=%p\n", i, ptr->type,
                        ptr->compiled_glsl_shader);

            // Check if glsl_program is valid
            if (!glsl_program)
            {
                DEBUG_PRINT("glsl_program is NULL!\n");
                ERROR_CHECK_RETURN(false, GL_INVALID_OPERATION);
            }

            glslang_program_add_shader(glsl_program, ptr->compiled_glsl_shader);
            DEBUG_PRINT("Successfully added shader stage %d to program\n", i);
        }
    }
}

char *parseSPIRVShaderToMetal(GLMContext ctx, Program *ptr, int stage)
{
    const SpvId *spirv;
    size_t word_count;
    char *str_ret;
    int parse_res;

    spvc_context context = NULL;
    spvc_parsed_ir ir = NULL;
    spvc_compiler compiler_msl = NULL;
    spvc_compiler_options options = NULL;
    spvc_resources resources = NULL;
    const spvc_reflected_resource *list = NULL;
    const char *result = NULL;
    size_t count;
    size_t i;

    spirv = ptr->spirv[stage].ir;
    assert(spirv);
    word_count = ptr->spirv[stage].size;
    assert(spirv);

    // Create context.
    spvc_context_create(&context);
    assert(context);

    // Set debug callback.
    spvc_context_set_error_callback(context, error_callback, ctx);

    // Parse the SPIR-V.
    parse_res = spvc_context_parse_spirv(context, spirv, word_count, &ir);
    assert(parse_res == SPVC_SUCCESS);

    // Hand it off to a compiler instance and give it ownership of the IR.
    spvc_context_create_compiler(context, SPVC_BACKEND_MSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler_msl);
    assert(compiler_msl);
    ERROR_CHECK_RETURN(spvc_compiler_msl_add_discrete_descriptor_set(compiler_msl, 3) == SPVC_SUCCESS,
                       GL_INVALID_OPERATION);

    // Modify options.
    ERROR_CHECK_RETURN(spvc_compiler_create_compiler_options(compiler_msl, &options) == SPVC_SUCCESS,
                       GL_INVALID_OPERATION);
    ERROR_CHECK_RETURN(spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_MSL_ARGUMENT_BUFFERS, SPVC_FALSE) ==
                           SPVC_SUCCESS,
                       GL_INVALID_OPERATION);
    ERROR_CHECK_RETURN(spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_MSL_VERSION,
                                                      SPVC_MAKE_MSL_VERSION(3, 1, 0)) == SPVC_SUCCESS,
                       GL_INVALID_OPERATION);
    // ERROR_CHECK_RETURN(spvc_compiler_options_set_uint(options,
    // SPVC_COMPILER_OPTION_GLSL_VERSION, 4.5) == SPVC_SUCCESS,
    // GL_INVALID_OPERATION);
    ERROR_CHECK_RETURN(spvc_compiler_install_compiler_options(compiler_msl, options) == SPVC_SUCCESS,
                       GL_INVALID_OPERATION);

    // create an entry point for metal based on the shader type and name
    GLuint name;
    char entry_point[128];
    name = ptr->shader_slots[stage]->name;

    SpvExecutionModel model;
    switch (stage)
    {
    case _VERTEX_SHADER:
        model = SpvExecutionModelVertex;
        break;
    case _TESS_CONTROL_SHADER:
        model = SpvExecutionModelTessellationControl;
        break;
    case _TESS_EVALUATION_SHADER:
        model = SpvExecutionModelTessellationEvaluation;
        break;
    case _GEOMETRY_SHADER:
        model = SpvExecutionModelGeometry;
        break;
    case _FRAGMENT_SHADER:
        model = SpvExecutionModelFragment;
        break;
    case _COMPUTE_SHADER:
        model = SpvExecutionModelGLCompute;
        break;
    default:
        assert(0);
    }

    switch (stage)
    {
    case _VERTEX_SHADER:
        snprintf(entry_point, sizeof(entry_point), "vertex_%d_main", name);
        break;
    case _TESS_CONTROL_SHADER:
        snprintf(entry_point, sizeof(entry_point), "tess_control_%d_main", name);
        break;
    case _TESS_EVALUATION_SHADER:
        snprintf(entry_point, sizeof(entry_point), "tess_evaluation_%d_main", name);
        break;
    case _GEOMETRY_SHADER:
        snprintf(entry_point, sizeof(entry_point), "geometry_%d", name);
        break;
    case _FRAGMENT_SHADER:
        snprintf(entry_point, sizeof(entry_point), "fragment_%d", name);
        break;
    case _COMPUTE_SHADER:
        snprintf(entry_point, sizeof(entry_point), "compute_%d", name);
        break;
    default:
        assert(0);
    }

    const char *cleansed_entry_point;
    cleansed_entry_point = spvc_compiler_get_cleansed_entry_point_name(compiler_msl, "main", model);

    spvc_result err;
    err = spvc_compiler_rename_entry_point(compiler_msl, cleansed_entry_point, entry_point, model);
    assert(err == SPVC_SUCCESS);

    // set the entry point for metal (in both shader and program so it survives detachment)
    ptr->shader_slots[stage]->entry_point = strdup(entry_point);
    ptr->mtl_data[stage].entry_point = strdup(entry_point);
    DEBUG_PRINT("SET entry_point for stage %d: %s\n", stage, entry_point);

    // compute shader
    if (stage == _COMPUTE_SHADER)
    {
        spvc_result res;
        const spvc_entry_point *entry_points;
        size_t num_entry_points;

        res = spvc_compiler_get_entry_points(compiler_msl, &entry_points, &num_entry_points);
        ERROR_CHECK_RETURN(res == SPVC_SUCCESS, GL_INVALID_OPERATION);

        for (int i = 0; i < num_entry_points; i++)
        {
            DEBUG_PRINT("Entry point: %s Execution Model: %d\n", entry_points[i].name, entry_points[i].execution_model);
        }

        ptr->local_workgroup_size.x =
            spvc_compiler_get_execution_mode_argument_by_index(compiler_msl, SpvExecutionModeLocalSize, 0);
        ptr->local_workgroup_size.y =
            spvc_compiler_get_execution_mode_argument_by_index(compiler_msl, SpvExecutionModeLocalSize, 1);
        ptr->local_workgroup_size.z =
            spvc_compiler_get_execution_mode_argument_by_index(compiler_msl, SpvExecutionModeLocalSize, 2);
    }

    // Do some basic reflection.
    spvc_compiler_create_shader_resources(compiler_msl, &resources);
    // Loop through all resource types including GL_PLAIN_UNIFORM (15)
    for (int res_type = SPVC_RESOURCE_TYPE_UNIFORM_BUFFER; res_type <= SPVC_RESOURCE_TYPE_GL_PLAIN_UNIFORM; res_type++)
    {
#if DEBUG
        const char *res_name[] = {
            "NONE",          "UNIFORM_BUFFER",  "UNIFORM_CONSTANT",  "STORAGE_BUFFER",         "STAGE_INPUT",
            "STAGE_OUTPUT",  "SUBPASS_INPUT",   "STORAGE_INPUT",     "SAMPLED_IMAGE",          "ATOMIC_COUNTER",
            "PUSH_CONSTANT", "SEPARATE_IMAGE",  "SEPARATE_SAMPLERS", "ACCELERATION_STRUCTURE", "RAY_QUERY",
            "SHADER_RECORD", "GL_PLAIN_UNIFORM"};
#endif

        spvc_resources_get_resource_list_for_type(resources, res_type, &list, &count);

        ptr->spirv_resources_list[stage][res_type].count = (GLuint)count;
        ptr->spirv_resources_list[stage][res_type].list = (SpirvResource *)malloc(count * sizeof(SpirvResource));

        for (i = 0; i < count; i++)
        {
            DEBUG_PRINT("res_type: %s ID: %u, BaseTypeID: %u, TypeID: %u, Name: %s ", res_name[res_type], list[i].id,
                        list[i].base_type_id, list[i].type_id, list[i].name);

            switch (res_type)
            {
            case SPVC_RESOURCE_TYPE_UNIFORM_BUFFER:
            case SPVC_RESOURCE_TYPE_PUSH_CONSTANT:
            case SPVC_RESOURCE_TYPE_STORAGE_BUFFER:
            case SPVC_RESOURCE_TYPE_ATOMIC_COUNTER:
                DEBUG_PRINT("Set: %u, Binding: %u Uniform: %d offset: %d\n",
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationDescriptorSet),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationBinding),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationUniform),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationOffset));
                break;

            case SPVC_RESOURCE_TYPE_STAGE_INPUT:
            case SPVC_RESOURCE_TYPE_STAGE_OUTPUT:
            case SPVC_RESOURCE_TYPE_SUBPASS_INPUT:
                DEBUG_PRINT("Set: %u, Location: %d Index: %d, offset: %d\n",
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationDescriptorSet),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationLocation),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationIndex),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationOffset));
                break;

            case SPVC_RESOURCE_TYPE_SAMPLED_IMAGE:
            case SPVC_RESOURCE_TYPE_SEPARATE_IMAGE:
                DEBUG_PRINT("Set: %u, Location: %d\n",
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationDescriptorSet),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationLocation));
                break;

            default:
                DEBUG_PRINT("Set: %u, Binding: %u Location: %d Index: %d, Uniform: %d "
                            "offset: %d\n",
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationDescriptorSet),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationBinding),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationLocation),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationIndex),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationUniform),
                            spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationOffset));
                break;
            }

            ptr->spirv_resources_list[stage][res_type].list[i]._id = list[i].id;
            ptr->spirv_resources_list[stage][res_type].list[i].base_type_id = list[i].base_type_id;
            ptr->spirv_resources_list[stage][res_type].list[i].type_id = list[i].type_id;
            ptr->spirv_resources_list[stage][res_type].list[i].name = strdup(list[i].name);
            ptr->spirv_resources_list[stage][res_type].list[i].set =
                spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationDescriptorSet);
            ptr->spirv_resources_list[stage][res_type].list[i].binding =
                spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationBinding);
            ptr->spirv_resources_list[stage][res_type].list[i].location =
                spvc_compiler_get_decoration(compiler_msl, list[i].id, SpvDecorationLocation);

            // Reflect uniform block members if this is a uniform buffer
            ptr->spirv_resources_list[stage][res_type].list[i].uniform_block = NULL;
            if (res_type == SPVC_RESOURCE_TYPE_UNIFORM_BUFFER)
            {
                spvc_type type = spvc_compiler_get_type_handle(compiler_msl, list[i].type_id);
                unsigned num_members = spvc_type_get_num_member_types(type);

                DEBUG_PRINT("  -> Uniform block '%s' has %u members:\n", list[i].name, num_members);

                if (num_members > 0)
                {
                    UniformBlockInfo *block_info = (UniformBlockInfo *)malloc(sizeof(UniformBlockInfo));
                    block_info->member_count = num_members;
                    block_info->members = (UniformBlockMember *)malloc(num_members * sizeof(UniformBlockMember));

                    for (unsigned m = 0; m < num_members; m++)
                    {
                        const char *member_name = spvc_compiler_get_member_name(compiler_msl, list[i].base_type_id, m);
                        unsigned member_offset = 0;
                        spvc_compiler_type_struct_member_offset(compiler_msl, type, m, &member_offset);

                        size_t member_size = 0;
                        spvc_compiler_get_declared_struct_member_size(compiler_msl, type, m, &member_size);

                        spvc_type_id member_type_id = spvc_type_get_member_type(type, m);

                        block_info->members[m].name = strdup(member_name ? member_name : "");
                        block_info->members[m].offset = member_offset;
                        block_info->members[m].size = (GLuint)member_size;
                        block_info->members[m].type_id = member_type_id;

                        DEBUG_PRINT("     [%u] %s: offset=%u, size=%zu\n", m, member_name ? member_name : "(unnamed)",
                                    member_offset, member_size);
                    }

                    ptr->spirv_resources_list[stage][res_type].list[i].uniform_block = block_info;
                }
            }
        }
    }

    spvc_compiler_compile(compiler_msl, &result);
    DEBUG_PRINT("\n%s\n", result);

    str_ret = strdup(result);

    // Frees all memory we allocated so far.
    spvc_context_destroy(context);

    return str_ret;
}

bool linkAndCompileProgramToMetal(GLMContext ctx, Program *pptr, int stage)
{
    glslang_program_t *glsl_program;
    int err;

    glsl_program = glslang_program_create();
    assert(glsl_program);

    // shaders to glsl program
    addShadersToProgram(ctx, pptr, glsl_program);

    // link
    err = glslang_program_link(glsl_program, GLSLANG_MSG_DEFAULT_BIT);
    if (!err)
    {
        // this is useful.. but information after this failure isn't that
        // interesting
        DEBUG_PRINT("glslang_program_link failed err: %d\n", err);
        DEBUG_PRINT("glslang_program_SPIRV_get_messages:\n%s\n", glslang_program_SPIRV_get_messages(glsl_program));
        DEBUG_PRINT("glslang_program_get_info_log:\n%s\n", glslang_program_get_info_log(glsl_program));
        DEBUG_PRINT("glslang_program_get_info_debug_log:\n%s\n", glslang_program_get_info_debug_log(glsl_program));

        ERROR_RETURN(GL_INVALID_OPERATION);
    }

    // generate SPIVR
    glslang_program_SPIRV_generate(glsl_program, stage);

    if (glslang_program_SPIRV_get_messages(glsl_program))
    {
        DEBUG_PRINT("%s\n", glslang_program_SPIRV_get_messages(glsl_program));

        ERROR_RETURN(GL_INVALID_OPERATION);
    }

    // save SPIRV code
    pptr->spirv[stage].size = glslang_program_SPIRV_get_size(glsl_program);
    pptr->spirv[stage].ir = (unsigned int *)malloc(pptr->spirv[stage].size * sizeof(unsigned));
    assert(pptr->spirv[stage].ir);
    glslang_program_SPIRV_get(glsl_program, pptr->spirv[stage].ir);

    // compile SPIRV to Metal
    pptr->spirv[stage].msl_str = parseSPIRVShaderToMetal(ctx, pptr, stage);
    ERROR_CHECK_RETURN(pptr->spirv[stage].msl_str, GL_INVALID_OPERATION);

    pptr->linked_glsl_program = glsl_program;
    pptr->dirty_bits |= DIRTY_PROGRAM;

    free(glsl_program);

    return true;
}

void mglLinkProgram(GLMContext ctx, GLuint program)
{
    Program *pptr;
    glslang_program_t *glsl_program;
    int err;

    pptr = findProgram(ctx, program);

    if (!pptr)
    {
        assert(0);

        return;
    }

    // Clean up any existing linked program
    if (pptr->linked_glsl_program)
    {
        glslang_program_delete(pptr->linked_glsl_program);
        pptr->linked_glsl_program = NULL;
    }

    // Create one glslang program for all shaders
    glsl_program = glslang_program_create();
    assert(glsl_program);

    // Add all shaders to the program
    addShadersToProgram(ctx, pptr, glsl_program);

    // Link the program once
    err = glslang_program_link(glsl_program, GLSLANG_MSG_DEFAULT_BIT);
    if (!err)
    {
        DEBUG_PRINT("glslang_program_link failed err: %d\n", err);
        DEBUG_PRINT("glslang_program_SPIRV_get_messages:\n%s\n", glslang_program_SPIRV_get_messages(glsl_program));
        DEBUG_PRINT("glslang_program_get_info_log:\n%s\n", glslang_program_get_info_log(glsl_program));
        DEBUG_PRINT("glslang_program_get_info_debug_log:\n%s\n", glslang_program_get_info_debug_log(glsl_program));

        glslang_program_delete(glsl_program);
        return;
    }

    // Generate SPIRV and compile to Metal for each shader stage
    for (int stage = 0; stage < _MAX_SHADER_TYPES; stage++)
    {
        pptr->spirv[stage].msl_str = 0;

        if (pptr->shader_slots[stage])
        {
            // Generate SPIRV for this stage
            glslang_program_SPIRV_generate(glsl_program, stage);

            if (glslang_program_SPIRV_get_messages(glsl_program))
            {
                DEBUG_PRINT("%s\n", glslang_program_SPIRV_get_messages(glsl_program));
                glslang_program_delete(glsl_program);
                return;
            }

            // Save SPIRV code
            pptr->spirv[stage].size = glslang_program_SPIRV_get_size(glsl_program);
            pptr->spirv[stage].ir = (unsigned int *)malloc(pptr->spirv[stage].size * sizeof(unsigned));
            assert(pptr->spirv[stage].ir);
            glslang_program_SPIRV_get(glsl_program, pptr->spirv[stage].ir);

            // Compile SPIRV to Metal
            pptr->spirv[stage].msl_str = parseSPIRVShaderToMetal(ctx, pptr, stage);
            if (!pptr->spirv[stage].msl_str)
            {
                glslang_program_delete(glsl_program);
                return;
            }
        }
    }

    // Save the linked glslang program
    pptr->linked_glsl_program = glsl_program;
    pptr->dirty_bits |= DIRTY_PROGRAM;

    ctx->mtl_funcs.mtlBindProgram(ctx, pptr);

    // ERROR_CHECK_RETURN(pptr->mtl_data, GL_INVALID_OPERATION);
}

void mglUseProgram(GLMContext ctx, GLuint program)
{
    Program *pptr;

    if (program)
    {
        pptr = findProgram(ctx, program);

        if (!pptr)
        {
            assert(0);

            return;
        }

        ERROR_CHECK_RETURN(pptr->linked_glsl_program, GL_INVALID_OPERATION);
    }
    else
    {
        pptr = NULL;
    }

    ctx->state.program = pptr;
    ctx->state.dirty_bits |= DIRTY_PROGRAM;
}

void mglBindAttribLocation(GLMContext ctx, GLuint program, GLuint index, const GLchar *name)
{
    // Unimplemented function
    assert(0);
}

void mglGetActiveAttrib(GLMContext ctx, GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size,
                        GLenum *type, GLchar *name)
{
    // Unimplemented function
    assert(0);
}

void mglGetActiveUniform(GLMContext ctx, GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size,
                         GLenum *type, GLchar *name)
{
    // Unimplemented function
    assert(0);
}

void mglGetAttachedShaders(GLMContext ctx, GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders)
{
    // Unimplemented function
    assert(0);
}

GLint mglGetAttribLocation(GLMContext ctx, GLuint program, const GLchar *name)
{
    if (isProgram(ctx, program) == GL_FALSE)
    {
        ERROR_RETURN(GL_INVALID_OPERATION); // also may be GL_INVALID_VALUE ????

        return -1;
    }

    Program *ptr;

    ptr = getProgram(ctx, program);
    assert(program);

    if (ptr->linked_glsl_program == NULL)
    {
        ERROR_RETURN(GL_INVALID_OPERATION);

        return -1;
    }

    for (int stage = _VERTEX_SHADER; stage < _MAX_SHADER_TYPES; stage++)
    {
        int count;

        count = ptr->spirv_resources_list[stage][SPVC_RESOURCE_TYPE_STAGE_INPUT].count;

        for (int i = 0; i < count; i++)
        {
            const char *str = ptr->spirv_resources_list[stage][SPVC_RESOURCE_TYPE_STAGE_INPUT].list[i].name;

            if (!strcmp(str, name))
            {
                GLuint location;

                location = ptr->spirv_resources_list[stage][SPVC_RESOURCE_TYPE_STAGE_INPUT].list[i].location;

                return location;
            }
        }
    }

    return -1;
}

void mglGetProgramiv(GLMContext ctx, GLuint program, GLenum pname, GLint *params)
{
    Program *ptr;

    ptr = findProgram(ctx, program);

    ERROR_CHECK_RETURN(ptr, GL_INVALID_VALUE);

    switch (pname)
    {
    case GL_DELETE_STATUS:
        *params = GL_FALSE;
        break;

    case GL_LINK_STATUS:
        if (ptr->linked_glsl_program)
        {
            *params = GL_TRUE;
        }
        else
        {
            *params = GL_FALSE;
        }
        break;

    case GL_VALIDATE_STATUS:
        *params = GL_TRUE;
        break;

    case GL_INFO_LOG_LENGTH:
        *params = 0;
        break;

    case GL_ATTACHED_SHADERS: {
        GLint count = 0;
        for (int i = 0; i < _MAX_SHADER_TYPES; i++)
        {
            if (ptr->shader_slots[i])
            {
                count++;
            }
        }
        *params = count;
    }
    break;

    case GL_ACTIVE_UNIFORMS: {
        GLint count = 0;
        for (int stage = 0; stage < _MAX_SHADER_TYPES; stage++)
        {
            count += ptr->spirv_resources_list[stage][SPVC_RESOURCE_TYPE_UNIFORM_BUFFER].count;
            count += ptr->spirv_resources_list[stage][SPVC_RESOURCE_TYPE_PUSH_CONSTANT].count;
        }
        *params = count;
    }
    break;

    case GL_ACTIVE_ATTRIBUTES: {
        GLint count = 0;
        for (int stage = 0; stage < _MAX_SHADER_TYPES; stage++)
        {
            count += ptr->spirv_resources_list[stage][SPVC_RESOURCE_TYPE_STAGE_INPUT].count;
        }
        *params = count;
    }
    break;

    default:
        ERROR_RETURN(GL_INVALID_ENUM);
        break;
    }
}

void mglGetProgramInfoLog(GLMContext ctx, GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog)
{
    // Unimplemented function
    assert(0);
}

#pragma mark program pipelines
void mglGenProgramPipelines(GLMContext ctx, GLsizei n, GLuint *pipelines)
{
    assert(0);
}

GLboolean mglIsProgramPipeline(GLMContext ctx, GLuint pipeline)
{
    assert(0);
}

void mglDeleteProgramPipelines(GLMContext ctx, GLsizei n, const GLuint *pipelines)
{
    assert(0);
}

void mglBindProgramPipeline(GLMContext ctx, GLuint pipeline)
{
    assert(0);
}

void mglUseProgramStages(GLMContext ctx, GLuint pipeline, GLbitfield stages, GLuint program)
{
    assert(0);
}
