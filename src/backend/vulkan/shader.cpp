#include "backend/vulkan/shader.h"

#include <fstream>

namespace pyroc::backend::vulkan
{
namespace
{
std::vector<char> readFile(const std::string& filename)
{
    std::vector<char> data;
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        // Failed oops
        return data;
    }

    std::streamsize fileSize = static_cast<std::streamsize>(file.tellg());
    data.resize(static_cast<uint32_t>(fileSize));
    file.seekg(0);
    file.read(data.data(), fileSize);

    file.close();

    return data;
}
}  // namespace

vk::ShaderModule createShaderFromFile(vk::Device device, const std::string& filename)
{
    const auto data = readFile(filename);

    return createShaderFromBytes(device, data);
}

vk::ShaderModule createShaderFromBytes(vk::Device device, const std::vector<char>& data)
{
    const vk::ShaderModuleCreateInfo createInfo = {
        .codeSize = data.size(),
        .pCode = reinterpret_cast<const uint32_t*>(data.data()),
    };

    const auto [res, shaderModule] = device.createShaderModule(createInfo);

    if (res != vk::Result::eSuccess)
    {
        return nullptr;
    }

    return shaderModule;
}
}  // namespace pyroc::backend::vulkan