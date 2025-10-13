#pragma once

#include "math/math.h"

namespace pyroc::core
{
struct Camera
{
    math::vec3 eye;
    math::vec3 center;
    math::vec3 up;
    float fovY;
    float aspect;
    float nearPlane;
    float farPlane;

    math::mat4 viewMatrix() const;
    math::mat4 projectionMatrix() const;
};
}  // namespace pyroc::core
