#include "window/window.h"

#include "backend/vulkan/api.h"

#include <iostream>

namespace pyroc
{

void Window::init()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mWindow = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    mBackend = new backend::vulkan::VulkanBackend();
    vk::Result res = mBackend->init(mWindow);
    if (res != vk::Result::eSuccess)
    {
        abort();
    }
}

void Window::loop()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
    }
}

void Window::cleanup()
{
    if (mBackend)
    {
        delete mBackend;
    }
    mBackend = nullptr;

    if (mWindow)
    {
        glfwDestroyWindow(mWindow);
    }
    mWindow = nullptr;

    glfwTerminate();
}
}  // namespace pyroc