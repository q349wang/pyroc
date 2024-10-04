#include "api.h"

#include <GLFW/glfw3.h>

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

std::vector<const char*> getRequiredExtensions()
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
}  // namespace

vk::Result VulkanBackend::init()
{
    const vk::ApplicationInfo appInfo{
        .pApplicationName = "PyroC Backend",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 0),
        .pEngineName = "PyroC",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    auto extensions = getRequiredExtensions();

    const bool useValidation = enableValidationLayers && checkValidationLayerSupport();

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
        const auto [res, devices] = mInstance.enumeratePhysicalDevices();
        if (res != vk::Result::eSuccess)
        {
            return res;
        }
        for (const auto& device : devices)
        {
            const auto properties = device.getProperties();

            if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                mPhysicalDevice = device;
            }
        }
    }

    {
        const auto queueFamilies = mPhysicalDevice.getQueueFamilyProperties();

        uint32_t i = 0;
        bool foundQueue = false;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags
                & (vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eGraphics))
            {
                foundQueue = true;
                mQueueFamilyIdx = i;
            }

            i++;
        }

        if (!foundQueue)
        {
            return vk::Result::eErrorIncompatibleDriver;
        }
    }

    {
        float queuePriority[1] = {1.0f};
        vk::DeviceQueueCreateInfo queueCreateInfo = {
            .queueFamilyIndex = mQueueFamilyIdx,
            .queueCount = 1,
            .pQueuePriorities = queuePriority,
        };

        vk::PhysicalDeviceFeatures deviceFeatures = {};

        vk::DeviceCreateInfo deviceCreateInfo = {
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
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
        mQueue = mDevice.getQueue(mQueueFamilyIdx, 0);
    }

    return vk::Result::eSuccess;
}

void VulkanBackend::destroy() { mInstance.destroy(); }

}  // namespace pyroc::backend::vulkan