#include "api.h"

#include "util/bitmanip.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

namespace pyroc::backend::vulkan
{

namespace
{
constexpr char const* validationLayers[1] = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

bool checkValidationLayerSupport()
{
    const auto [res, availableLayers] = vk::enumerateInstanceLayerProperties();
    if (res != vk::Result::eSuccess)
    {
        return false;
    }

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> getRequiredInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

std::vector<const char*> getRequiredDeviceExtensions()
{
    std::vector<const char*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    return extensions;
}

uint32_t findBestQueue(vk::QueueFlags desiredFlags,
                       const std::vector<vk::QueueFamilyProperties>& queueFamilies)
{
    uint32_t targetIndex = std::numeric_limits<uint32_t>::max();
    vk::QueueFlags targetFlags = vk::QueueFlags(~0u);

    for (uint32_t i = 0; i < queueFamilies.size(); ++i)
    {
        // Ignore flags not needed
        if (vk::QueueFlags(0) == (queueFamilies[i].queueFlags & desiredFlags))
        {
            continue;
        }

        // Matching queue, but with fewer flags than the current best?  Use it.
        if (countBits(vk::QueueFlags::MaskType(queueFamilies[i].queueFlags))
            < countBits(vk::QueueFlags::MaskType(targetFlags)))
        {
            targetIndex = i;
            targetFlags = queueFamilies[i].queueFlags;
        }
    }

    return targetIndex;
}

VulkanBackend::QueueFamilyIndices findQueueFamiles(vk::PhysicalDevice device,
                                                   vk::SurfaceKHR surface)
{
    VulkanBackend::QueueFamilyIndices indices;
    const auto queueFamilies = device.getQueueFamilyProperties();

    indices.graphics = findBestQueue(vk::QueueFlagBits::eGraphics, queueFamilies);

    for (uint32_t i = 0; i < queueFamilies.size(); ++i)
    {
        const auto [res, presentSupport] = device.getSurfaceSupportKHR(i, surface);
        if (res != vk::Result::eSuccess)
        {
            break;
        }

        if (presentSupport)
        {
            indices.present = i;
        }
    }

    return indices;
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice device)
{
    const auto [res, availableExtensions] = device.enumerateDeviceExtensionProperties();

    if (res != vk::Result::eSuccess)
    {
        return false;
    }

    const auto requiredExtensions = getRequiredDeviceExtensions();

    // Just gonna do a n^2 search I CBA
    for (const auto& extensionName : requiredExtensions)
    {
        bool found = false;
        for (const auto& availExtension : availableExtensions)
        {
            if (!strncmp(extensionName, availExtension.extensionName, strlen(extensionName)))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            return false;
        }
    }
    return true;
}

VulkanBackend::SwapchainInfo querySwapchainInfo(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    VulkanBackend::SwapchainInfo info;

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

bool isDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    const auto properties = device.getProperties();

    if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
    {
        return false;
    }

    const auto queueFamily = findQueueFamiles(device, surface);
    if (queueFamily.graphics == std::numeric_limits<uint32_t>::max()
        || queueFamily.present == std::numeric_limits<uint32_t>::max())
    {
        return false;
    }

    const bool extSupport = checkDeviceExtensionSupport(device);
    if (!extSupport)
    {
        return false;
    }

    const auto swapchainInfo = querySwapchainInfo(device, surface);
    if (swapchainInfo.formats.empty() || swapchainInfo.presentModes.empty())
    {
        return false;
    }

    return true;
}

vk::SurfaceFormatKHR chooseSwapchainFormat(const VulkanBackend::SwapchainInfo& info)
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

vk::PresentModeKHR chooseSwapchainPresentMode(const VulkanBackend::SwapchainInfo& info)
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

vk::Extent2D chooseSwapchainExtent(GLFWwindow* window, const VulkanBackend::SwapchainInfo& info)
{
    if (info.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return info.capabilities.currentExtent;
    }

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExtent = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
    };

    actualExtent.width = std::clamp(actualExtent.width, info.capabilities.minImageExtent.width,
                                    info.capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, info.capabilities.minImageExtent.height,
                                     info.capabilities.maxImageExtent.height);

    return actualExtent;
}

}  // namespace

vk::Result VulkanBackend::init(GLFWwindow* window)
{
    const vk::ApplicationInfo appInfo{
        .pApplicationName = "PyroC Backend",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 0),
        .pEngineName = "PyroC",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    auto extensions = getRequiredInstanceExtensions();

    const bool useValidation = enableValidationLayers && checkValidationLayerSupport();
    std::cout << "Using validation?: " << (useValidation ? "TRUE" : "FALSE") << std::endl;

    {
        const vk::InstanceCreateInfo instanceCreateInfo = {
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = useValidation ? static_cast<uint32_t>(
                                     sizeof(validationLayers) / sizeof(validationLayers[0]))
                                               : 0,
            .ppEnabledLayerNames = useValidation ? validationLayers : nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        auto [res, instance] = vk::createInstance(instanceCreateInfo);
        if (res != vk::Result::eSuccess)
        {
            return res;
        }
        mInstance = instance;
    }

    {
        VkSurfaceKHR surface;
        auto res = glfwCreateWindowSurface(mInstance, window, nullptr, &surface);
        if (res != VK_SUCCESS)
        {
            return vk::Result(res);
        }

        mSurface = surface;
    }

    {
        const auto [res, devices] = mInstance.enumeratePhysicalDevices();
        if (res != vk::Result::eSuccess)
        {
            return res;
        }

        bool found = false;
        for (const auto& device : devices)
        {
            if (isDeviceSuitable(device, mSurface))
            {
                found = true;
                mPhysicalDevice = device;
                break;
            }
        }

        if (!found)
        {
            return vk::Result::eErrorIncompatibleDriver;
        }
    }

    {
        mQueueFamilyIndices = findQueueFamiles(mPhysicalDevice, mSurface);
    }

    {
        float queuePriority[1] = {1.0f};

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

        queueCreateInfos.push_back({
            .queueFamilyIndex = mQueueFamilyIndices.graphics,
            .queueCount = 1,
            .pQueuePriorities = queuePriority,
        });

        if (mQueueFamilyIndices.graphics != mQueueFamilyIndices.present)
        {
            queueCreateInfos.push_back({
                .queueFamilyIndex = mQueueFamilyIndices.present,
                .queueCount = 1,
                .pQueuePriorities = queuePriority,
            });
        }

        const vk::PhysicalDeviceFeatures deviceFeatures = {};

        const auto requiredExts = getRequiredDeviceExtensions();

        const vk::DeviceCreateInfo deviceCreateInfo = {
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExts.size()),
            .ppEnabledExtensionNames = requiredExts.data(),
            .pEnabledFeatures = &deviceFeatures,
        };

        const auto [res, device] = mPhysicalDevice.createDevice(deviceCreateInfo);
        if (res != vk::Result::eSuccess)
        {
            return res;
        }

        mDevice = device;
    }

    {
        mGraphicsQueue = mDevice.getQueue(mQueueFamilyIndices.graphics, 0);
        mPresentQueue = mDevice.getQueue(mQueueFamilyIndices.present, 0);
    }

    {
        const SwapchainInfo info = querySwapchainInfo(mPhysicalDevice, mSurface);

        const vk::SurfaceFormatKHR format = chooseSwapchainFormat(info);
        const vk::PresentModeKHR presentMode = chooseSwapchainPresentMode(info);
        const vk::Extent2D extent = chooseSwapchainExtent(window, info);

        uint32_t imageCount = info.capabilities.minImageCount + 1;

        if (info.capabilities.maxImageCount > 0 && imageCount > info.capabilities.maxImageCount)
        {
            imageCount = info.capabilities.maxImageCount;
        }

        const bool exclusiveQueue = mQueueFamilyIndices.graphics == mQueueFamilyIndices.present;
        const uint32_t queueFamilyIndices[]
            = {mQueueFamilyIndices.graphics, mQueueFamilyIndices.present};
        const vk::SwapchainCreateInfoKHR createInfo = {
            .surface = mSurface,
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
            .clipped = vk::True,
            .oldSwapchain = nullptr,
        };

        const auto [res, swapchain] = mDevice.createSwapchainKHR(createInfo);
        if (res != vk::Result::eSuccess)
        {
            return res;
        }

        mSwapchain = swapchain;
        mSwapchainFormat = format.format;
        mSwapchainExtent = extent;
    }

    {
        const auto [res, swapchainImages] = mDevice.getSwapchainImagesKHR(mSwapchain);
        if (res != vk::Result::eSuccess)
        {
            return res;
        }

        mSwapchainImages = swapchainImages;
    }

    return vk::Result::eSuccess;
}

void VulkanBackend::destroy()
{
    mDevice.destroySwapchainKHR(mSwapchain);
    mDevice.destroy();
    mInstance.destroySurfaceKHR(mSurface);
    mInstance.destroy();
}

}  // namespace pyroc::backend::vulkan