#include "core/camera.h"

namespace pyroc::core
{
math::mat4 Camera::viewMatrix() const
{
    const math::vec3 f = math::normalize(center - eye);
    const math::vec3 s = math::normalize(math::cross(f, up));
    const math::vec3 u = math::normalize(math::cross(s, f));

    math::mat4 result = math::mat4::identity();
    result[0][0] = s.x;
    result[0][1] = s.y;
    result[0][2] = s.z;
    result[1][0] = u.x;
    result[1][1] = u.y;
    result[1][2] = u.z;
    result[2][0] = f.x;
    result[2][1] = f.y;
    result[2][2] = f.z;
    result[3][0] = -math::dot(s, eye);
    result[3][1] = -math::dot(u, eye);
    result[3][2] = -math::dot(f, eye);
    return result;
}

/*
 * NDC X: -1 to 1
 * NDC Y: -1 to 1
 * NDC Z: 0 to 1
 * RHS coordinate system
 *Origin top left
 */
math::mat4 Camera::projectionMatrix() const
{
    return math::perspective(math::radians(fovY), aspect, nearPlane, farPlane);
}
}  // namespace pyroc::core
