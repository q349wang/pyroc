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

vk::Result copyBufferBlocking(Context* ctx, vk::CommandBuffer cmdBuf, vk::DeviceSize srcOffset,
                              Buffer& src, vk::DeviceSize dstOffset, Buffer& dst)
{
    vk::Result res;

    res = cmdBuf.reset();

    if (res != vk::Result::eSuccess)
    {
        return res;
    }

    {
        const vk::CommandBufferBeginInfo beginInfo = {
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        };

        res = cmdBuf.begin(beginInfo);

        if (res != vk::Result::eSuccess)
        {
            return res;
        }
    }

    {
        vk::BufferCopy copyRegion = {
            .srcOffset = srcOffset,
            .dstOffset = dstOffset,
            .size = src.size,
        };

        cmdBuf.copyBuffer(src.buffer, dst.buffer, copyRegion);
    }

    {
        res = cmdBuf.end();

        if (res != vk::Result::eSuccess)
        {
            return res;
        }
    }

    {
        const vk::SubmitInfo submitInfo = {
            .commandBufferCount = 1,
            .pCommandBuffers = &cmdBuf,
        };

        res = ctx->graphicsQueue().submit(submitInfo);

        if (res != vk::Result::eSuccess)
        {
            return res;
        }
    }

    {
        res = ctx->graphicsQueue().waitIdle();

        if (res != vk::Result::eSuccess)
        {
            return res;
        }
    }

    return vk::Result::eSuccess;
}

vk::Result copyBufferBlocking(Context* ctx, vk::CommandBuffer cmdBuf, vk::DeviceSize srcSize,
                              const void* pSrc, vk::DeviceSize dstOffset, Buffer& dst)
{
    vk::Result res;

    Buffer srcBuffer;
    res = createBuffer(
        ctx, srcSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        srcBuffer);

    if (res != vk::Result::eSuccess)
    {
        return res;
    }

    {
        void* pData;
        res = ctx->device().mapMemory(srcBuffer.memory, 0, srcSize, {}, &pData);

        if (res != vk::Result::eSuccess)
        {
            destroyBuffer(ctx, srcBuffer);
            return res;
        }

        std::memcpy(pData, pSrc, srcSize);

        ctx->device().unmapMemory(srcBuffer.memory);
    }

    res = copyBufferBlocking(ctx, cmdBuf, 0, srcBuffer, dstOffset, dst);

    destroyBuffer(ctx, srcBuffer);

    return res;
}

}  // namespace pyroc::backend::vulkan