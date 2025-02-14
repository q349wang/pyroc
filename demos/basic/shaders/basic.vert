#version 460

#include "basic.h"

layout(location = IN_VERTEX_POSITION) in vec2 inPosition;
layout(location = IN_VERTEX_COLOUR) in vec3 inColour;

layout(location = OUT_FRAG_COLOUR) out vec3 fragColour;

void main()
{
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColour = inColour;
}