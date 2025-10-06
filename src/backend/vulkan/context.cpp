#include "backend/vulkan/context.h"
#include "backend/vulkan/api.h"

#include "util/bitmanip.h"
#include "util/log.h"

#include <algorithm>
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

vk::Bool32 getPresentationSupport(vk::PhysicalDevice device, uint32_t queueFamilyIndex)
{
#ifdef _WIN32
    return device.getWin32PresentationSupportKHR(queueFamilyIndex);
#else
    #error "Unsupported operating system"
#endif
}

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
        extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
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
        if (util::countBits(vk::QueueFlags::MaskType(queueFamilies[i].queueFlags))
            < util::countBits(vk::QueueFlags::MaskType(targetFlags)))
        {
            targetIndex = i;
            targetFlags = queueFamilies[i].queueFlags;
        }
    }

    return targetIndex;
}

Context::QueueFamilyIndices findQueueFamiles(vk::PhysicalDevice device)
{
    Context::QueueFamilyIndices indices;
    const auto queueFamilies = device.getQueueFamilyProperties();

    indices.graphics = findBestQueue(vk::QueueFlagBits::eGraphics, queueFamilies);

    if (getPresentationSupport(device, indices.graphics))
    {
        indices.present = indices.graphics;  // Graphics and present are the same queue
        return indices;
    }

    for (uint32_t i = 0; i < queueFamilies.size(); ++i)
    {
        const vk::Bool32 presentSupport = getPresentationSupport(device, i);

        if (presentSupport)
        {
            indices.present = i;
            break;
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

bool isDeviceSuitable(vk::PhysicalDevice device)
{
    const auto properties = device.getProperties();

    if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
    {
        return false;
    }

    const auto queueFamily = findQueueFamiles(device);
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

    return true;
}

}  // namespace

vk::Result Context::init()
{
    glfwInit();

    const vk::ApplicationInfo appInfo{
        .pApplicationName = "PyroC Backend",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 0),
        .pEngineName = "PyroC",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    const bool useValidation = enableValidationLayers && checkValidationLayerSupport();
    LOG_DEBUG("Using validation?: %s", (useValidation ? "TRUE" : "FALSE"));

    auto extensions = getRequiredInstanceExtensions();

    {
        const vk::ValidationFeatureEnableEXT validationFeaturesEnabled[] = {
            vk::ValidationFeatureEnableEXT::eGpuAssisted,
            vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot,
            vk::ValidationFeatureEnableEXT::eBestPractices,
            vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
        };

        const vk::ValidationFeaturesEXT layerSettingsCreateInfo = {
            .enabledValidationFeatureCount
            = useValidation ? static_cast<uint32_t>(std::size(validationFeaturesEnabled)) : 0,
            .pEnabledValidationFeatures = useValidation ? validationFeaturesEnabled : nullptr,
        };

        const vk::LayerSettingsCreateInfoEXT layerSettings = {
            .pNext = useValidation ? &layerSettingsCreateInfo : nullptr,
            .settingCount = 0,
            .pSettings = nullptr,
        };

        const vk::InstanceCreateInfo instanceCreateInfo = {
            .pNext = &layerSettings,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount
            = useValidation ? static_cast<uint32_t>(std::size(validationLayers)) : 0,
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
        const auto [res, devices] = mInstance.enumeratePhysicalDevices();
        if (res != vk::Result::eSuccess)
        {
            return res;
        }

        bool found = false;
        for (const auto& device : devices)
        {
            if (isDeviceSuitable(device))
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
        mQueueFamilyIndices = findQueueFamiles(mPhysicalDevice);
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

    return vk::Result::eSuccess;
}

void Context::destroy()
{
    mDevice.destroy();
    mInstance.destroy();

    glfwTerminate();
}

}  // namespace pyroc::backend::vulkan
