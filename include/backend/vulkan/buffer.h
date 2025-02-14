#pragma once

#include "api.h"
#include "context.h"

namespace pyroc::backend::vulkan
{

struct Buffer
{
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags properties;
};

vk::Result createBuffer(Context* ctx, vk::DeviceSize size, vk::BufferUsageFlags usage,
                        vk::MemoryPropertyFlags properties, Buffer& buffer);
void destroyBuffer(Context* ctx, Buffer& buffer);
}  // namespace pyroc::backend::vulkan
