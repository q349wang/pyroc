#include "window/window.h"

#include "backend/vulkan/context.h"

#include <iostream>

namespace pyroc
{

void Window::init()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mWindow = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    mContext = new backend::vulkan::Context();
    vk::Result res = mContext->init(mWindow);
    if (res != vk::Result::eSuccess)
    {
        abort();
    }
}

void Window::cleanup()
{
    if (mContext)
    {
        delete mContext;
    }
    mContext = nullptr;

    if (mWindow)
    {
        glfwDestroyWindow(mWindow);
    }
    mWindow = nullptr;

    glfwTerminate();
}
}  // namespace pyroc