#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

namespace pyroc::backend::vulkan
{
class VulkanBackend
{
  public:
    vk::Result init();

    void destroy();

  private:
    vk::Instance mInstance;
    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mDevice;
    uint32_t mQueueFamilyIdx;
    vk::Queue mQueue;
};

}  // namespace pyroc::backend::vulkan