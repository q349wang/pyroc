#pragma once

#include "api.h"

namespace pyroc::backend::vulkan
{

class Context
{
  public:
    struct QueueFamilyIndices
    {
        uint32_t graphics = ~0u;
        uint32_t present = ~0u;
    };

    vk::Result init();

    void destroy();

    vk::Instance instance() { return mInstance; }

    vk::PhysicalDevice physicalDevice() { return mPhysicalDevice; }
    vk::Device device() { return mDevice; }

    uint32_t graphicsQueueIdx() { return mQueueFamilyIndices.graphics; }
    uint32_t presentQueueIdx() { return mQueueFamilyIndices.present; }

    vk::Queue graphicsQueue() { return mGraphicsQueue; }
    vk::Queue presentQueue() { return mPresentQueue; }

  private:
    vk::Instance mInstance;

    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mDevice;

    QueueFamilyIndices mQueueFamilyIndices;
    vk::Queue mGraphicsQueue;
    vk::Queue mPresentQueue;
};

}  // namespace pyroc::backend::vulkan