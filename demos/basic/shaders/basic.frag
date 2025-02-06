#version 450

#include "basic.h"

layout(location = OUT_FRAG_COLOUR) in vec3 fragColour;

layout(location = 0) out vec4 outColour;

void main() { outColour = vec4(fragColour, 1.0); }