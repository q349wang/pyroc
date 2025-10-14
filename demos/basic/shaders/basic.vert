#version 460
#extension GL_EXT_debug_printf : enable

#include "basic.h"

layout(location = IN_VERTEX_POSITION) in vec3 inPosition;
layout(location = IN_VERTEX_COLOUR) in vec3 inColour;

layout(location = OUT_FRAG_COLOUR) out vec3 fragColour;

layout(push_constant) uniform constants{PUSH_CONSTANTS} pc;

void main()
{
    vec4 modelPosition = pc.view * pc.model * vec4(inPosition, 1.0);
    debugPrintfEXT("inPos %f %f %f modelPos %f %f %f\n", inPosition.x, inPosition.y, inPosition.z,
                   modelPosition.x, modelPosition.y, modelPosition.z);
    gl_Position = modelPosition;
    fragColour = inColour;
}
