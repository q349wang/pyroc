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

    struct SwapchainInfo
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    vk::Result init(GLFWwindow* window);

    vk::Result recreateSwapchain(GLFWwindow* window);

    void destroy();

    vk::PhysicalDevice physicalDevice() { return mPhysicalDevice; }
    vk::Device device() { return mDevice; }

    uint32_t graphicsQueueIdx() { return mQueueFamilyIndices.graphics; }

    vk::Queue graphicsQueue() { return mGraphicsQueue; }
    vk::Queue presentQueue() { return mPresentQueue; }

    vk::SwapchainKHR swapchain() { return mSwapchain; }
    vk::Format swapchainFormat() { return mSwapchainFormat; }

    vk::Extent2D swapchainExtent() { return mSwapchainExtent; }

    std::vector<vk::ImageView> swapchainImageViews() { return mSwapchainImageViews; }

  private:
    vk::Result createSwapchain(GLFWwindow* window);
    void destroySwapchain();

    vk::Instance mInstance;

    vk::SurfaceKHR mSurface;

    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mDevice;

    QueueFamilyIndices mQueueFamilyIndices;
    vk::Queue mGraphicsQueue;
    vk::Queue mPresentQueue;

    vk::SwapchainKHR mSwapchain;
    vk::Format mSwapchainFormat;
    vk::Extent2D mSwapchainExtent;
    std::vector<vk::Image> mSwapchainImages;

    std::vector<vk::ImageView> mSwapchainImageViews;
};

}  // namespace pyroc::backend::vulkan