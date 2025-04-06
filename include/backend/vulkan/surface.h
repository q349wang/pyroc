#pragma once

#include "api.h"

namespace pyroc::backend::vulkan
{

class Context;

struct Surface
{
    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapchain;
    vk::Format format;
    vk::Extent2D extent;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::ImageView> swapchainImageViews;
};

struct SurfaceCreateInfo
{
    vk::SurfaceKHR surface;
    vk::Extent2D extent;
};

vk::Result createSurface(Context* ctx, const SurfaceCreateInfo* createInfo, Surface& surface);
vk::Result recreateSurface(Context* ctx, const SurfaceCreateInfo* createInfo, Surface& surface);
void destroySurface(Context* ctx, Surface& surface);

}  // namespace pyroc::backend::vulkan