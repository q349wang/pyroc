#version 460

#include "basic.h"

layout(location = IN_VERTEX_POSITION) in vec2 inPosition;
layout(location = IN_VERTEX_COLOUR) in vec3 inColour;

layout(location = OUT_FRAG_COLOUR) out vec3 fragColour;

vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));

vec3 colours[3] = vec3[](vec3(2.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColour = colours[gl_VertexIndex];
}