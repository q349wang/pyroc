#include "backend/vulkan/buffer.h"

namespace pyroc::backend::vulkan
{
vk::Result createBuffer(Context* ctx, vk::DeviceSize size, vk::BufferUsageFlags usage,
                        vk::MemoryPropertyFlags properties, Buffer& buffer)
{
    vk::Device device = ctx->device();
    vk::BufferCreateInfo bufferInfo = {
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };

    vk::Buffer bufferHandle;
    vk::DeviceMemory memoryHandle;
    {
        const auto [res, handle] = device.createBuffer(bufferInfo);
        if (res != vk::Result::eSuccess)
        {
            return res;
        }

        bufferHandle = handle;
    }

    vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(bufferHandle);

    vk::PhysicalDeviceMemoryProperties memProperties = ctx->physicalDevice().getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((memRequirements.memoryTypeBits & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            vk::MemoryAllocateInfo allocInfo = {
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = i,
            };

            {
                const auto [res, handle] = device.allocateMemory(allocInfo);
                if (res != vk::Result::eSuccess)
                {
                    device.destroyBuffer(bufferHandle);
                    return res;
                }
                memoryHandle = handle;
            }

            {
                const auto res = device.bindBufferMemory(bufferHandle, memoryHandle, 0);

                if (res != vk::Result::eSuccess)
                {
                    device.freeMemory(memoryHandle);
                    device.destroyBuffer(bufferHandle);

                    return res;
                }
            }

            buffer.size = size;
            buffer.usage = usage;
            buffer.properties = properties;
            buffer.memory = memoryHandle;
            buffer.buffer = bufferHandle;

            return vk::Result::eSuccess;
        }
    }

    return vk::Result::eErrorUnknown;
}
void destroyBuffer(Context* ctx, Buffer& buffer)
{
    ctx->device().freeMemory(buffer.memory);
    ctx->device().destroyBuffer(buffer.buffer);

    return;
}
}  // namespace pyroc::backend::vulkan