#ifndef SHADER_BASIC_H
#define SHADER_BASIC_H

#define IN_BINDING_VERTEX 0
#define IN_VERTEX_POSITION 0
#define IN_VERTEX_COLOUR 1

#define OUT_FRAG_COLOUR 0

#define PUSH_CONSTANTS \
    mat4 model;        \
    mat4 view;         \
    mat4 projection;

#ifdef __cplusplus
    #include "pyroc.h"

using namespace pyroc::math;

struct PushConstants
{
    PUSH_CONSTANTS
};
#endif

#endif
