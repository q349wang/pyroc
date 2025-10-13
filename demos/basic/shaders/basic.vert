#version 460

#include "basic.h"

layout(location = IN_VERTEX_POSITION) in vec3 inPosition;
layout(location = IN_VERTEX_COLOUR) in vec3 inColour;

layout(location = OUT_FRAG_COLOUR) out vec3 fragColour;

layout(push_constant) uniform constants{PUSH_CONSTANTS} pc;

void main()
{
    gl_Position = vec4(inPosition.x, inPosition.y, 0.0, 1.0);
    fragColour = inColour;
}
