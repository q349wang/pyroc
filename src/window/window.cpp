#include "window/window.h"

#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <iostream>

namespace pyroc
{
void Window::init()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    const vk::Result res
        = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    if (res != vk::Result::eSuccess)
    {
        abort();
    }

    std::cout << extensionCount << " extensions supported\n";

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}
}  // namespace pyroc