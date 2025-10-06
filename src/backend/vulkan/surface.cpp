#include "backend/vulkan/surface.h"

#include "backend/vulkan/context.h"

#include "util/log.h"

#include <limits>

namespace pyroc::backend::vulkan
{

namespace
{
struct SwapchainInfo
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

SwapchainInfo querySwapchainInfo(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    SwapchainInfo info;

    {
        const auto [res, capabilities] = device.getSurfaceCapabilitiesKHR(surface);
        if (res == vk::Result::eSuccess)
        {
            info.capabilities = capabilities;
        }
    }

    {
        const auto [res, formats] = device.getSurfaceFormatsKHR(surface);

        info.formats = formats;
    }

    {
        const auto [res, presentModes] = device.getSurfacePresentModesKHR(surface);

        info.presentModes = presentModes;
    }

    return info;
}

vk::SurfaceFormatKHR chooseSwapchainFormat(const SwapchainInfo& info)
{
    for (const auto& format : info.formats)
    {
        if (format.format == vk::Format::eB8G8R8A8Srgb
            && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }

    return info.formats[0];
}

vk::PresentModeKHR chooseSwapchainPresentMode(const SwapchainInfo& info)
{
    for (const auto& presentMode : info.presentModes)
    {
        if (presentMode == vk::PresentModeKHR::eMailbox)
        {
            return presentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapchainExtent(const vk::Extent2D& extent, const SwapchainInfo& info)
{
    if (info.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return info.capabilities.currentExtent;
    }

    vk::Extent2D actualExtent = extent;

    actualExtent.width = std::clamp(actualExtent.width, info.capabilities.minImageExtent.width,
                                    info.capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, info.capabilities.minImageExtent.height,
                                     info.capabilities.maxImageExtent.height);

    return actualExtent;
}

vk::Result createSwapchain(Context* ctx, Surface& surface)
{
    vk::PhysicalDevice physicalDevice = ctx->physicalDevice();
    vk::Device device = ctx->device();

    {
        const SwapchainInfo info = querySwapchainInfo(physicalDevice, surface.surface);

        const vk::SurfaceFormatKHR format = chooseSwapchainFormat(info);
        const vk::PresentModeKHR presentMode = chooseSwapchainPresentMode(info);
        const vk::Extent2D extent = chooseSwapchainExtent(surface.extent, info);

        if (extent.width == 0 || extent.height == 0)
        {
            // Surface extent is invalid, skip creating the swapchain
            return vk::Result::eSuccess;
        }

        LOG_DEBUG("Creating swapchain with format: %s, present mode: %s, extent: (%ux%u)",
                  vk::to_string(format.format).c_str(), vk::to_string(presentMode).c_str(),
                  extent.width, extent.height);

        uint32_t imageCount = info.capabilities.minImageCount + 1;

        if (info.capabilities.maxImageCount > 0 && imageCount > info.capabilities.maxImageCount)
        {
            imageCount = info.capabilities.maxImageCount;
        }

        const uint32_t queueFamilyIndices[] = {ctx->graphicsQueueIdx(), ctx->presentQueueIdx()};
        const bool exclusiveQueue = queueFamilyIndices[0] == queueFamilyIndices[1];

        const vk::SwapchainCreateInfoKHR createInfo = {
            .surface = surface.surface,
            .minImageCount = imageCount,
            .imageFormat = format.format,
            .imageColorSpace = format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode
            = exclusiveQueue ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
            .queueFamilyIndexCount = exclusiveQueue ? 0u : 2u,
            .pQueueFamilyIndices = exclusiveQueue ? nullptr : queueFamilyIndices,
            .preTransform = info.capabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr,
        };

        const auto [res, swapchain] = device.createSwapchainKHR(createInfo);
        if (res != vk::Result::eSuccess)
        {
            return res;
        }

        surface.swapchain = swapchain;
        surface.format = format.format;
    }

    {
        const auto [res, swapchainImages] = device.getSwapchainImagesKHR(surface.swapchain);
        if (res != vk::Result::eSuccess)
        {
            return res;
        }

        surface.swapchainImages = swapchainImages;
    }

    {
        surface.swapchainImageViews.resize(surface.swapchainImages.size());
        for (uint32_t i = 0; i < surface.swapchainImages.size(); i++)
        {
            vk::ImageViewCreateInfo createInfo = {
                .image = surface.swapchainImages[i],
                .viewType = vk::ImageViewType::e2D,
                .format = surface.format,
                .components = {
                    .r = vk::ComponentSwizzle::eIdentity,
                    .g = vk::ComponentSwizzle::eIdentity,
                    .b = vk::ComponentSwizzle::eIdentity,
                    .a = vk::ComponentSwizzle::eIdentity,
                },
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            const auto [res, image] = device.createImageView(createInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }

            surface.swapchainImageViews[i] = image;
        }
    }

    return vk::Result::eSuccess;
}

void destroySwapchain(Context* ctx, Surface& surface)
{
    vk::Device device = ctx->device();

    for (auto& imageView : surface.swapchainImageViews)
    {
        device.destroyImageView(imageView);
        imageView = nullptr;
    }
    device.destroySwapchainKHR(surface.swapchain);
    surface.swapchain = nullptr;
}

}  // namespace

vk::Result createSurface(Context* ctx, const SurfaceCreateInfo* createInfo, Surface& surface)
{
    surface.surface = createInfo->surface;
    surface.extent = createInfo->extent;

    if (surface.extent.width == 0 || surface.extent.height == 0)
    {
        return vk::Result::eSuccess;
    }

    return createSwapchain(ctx, surface);
}

vk::Result recreateSurface(Context* ctx, const SurfaceCreateInfo* createInfo, Surface& surface)
{
    vk::Device device = ctx->device();
    const auto res = device.waitIdle();
    if (res != vk::Result::eSuccess)
    {
        return res;
    }

    surface.extent = createInfo->extent;

    destroySwapchain(ctx, surface);

    if (surface.extent.width == 0 || surface.extent.height == 0)
    {
        return vk::Result::eSuccess;
    }
    return createSwapchain(ctx, surface);
}

void destroySurface(Context* ctx, Surface& surface)
{
    destroySwapchain(ctx, surface);
    surface.surface = nullptr;
    surface.swapchain = nullptr;
    surface.format = vk::Format::eUndefined;
    surface.extent = vk::Extent2D{};
    surface.swapchainImages.clear();
    surface.swapchainImageViews.clear();
}
}  // namespace pyroc::backend::vulkan
