#pragma once

#include "backend\vulkan\api.h"

#include <string>
#include <vector>

namespace pyroc::backend::vulkan
{
vk::ShaderModule createShaderFromFile(vk::Device device, const std::string& filename);

vk::ShaderModule createShaderFromBytes(vk::Device device, const std::vector<char>& data);

}  // namespace pyroc::backend::vulkan