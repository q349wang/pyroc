#include "window/window.h"

#include "backend/vulkan/context.h"

namespace pyroc::window
{

void Window::init(const WindowCreateInfo* createInfo)
{
    mWidth = createInfo->width;
    mHeight = createInfo->height;
    mName = createInfo->name ? createInfo->name : "Pyroc Window";
    mMode = createInfo->mode;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    int32_t width = static_cast<int32_t>(mWidth);
    int32_t height = static_cast<int32_t>(mHeight);

    switch (mMode)
    {
        case WindowMode::eWindowed:
        {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            mWindow = glfwCreateWindow(width, height, mName.c_str(), nullptr, nullptr);
            glfwSetWindowAttrib(mWindow, GLFW_DECORATED, GLFW_TRUE);

            break;
        }
        case WindowMode::eFullscreen:
        {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            mWindow = glfwCreateWindow(width, height, mName.c_str(), monitor, nullptr);

            break;
        }
        case WindowMode::eBorderlessWindowed:
        {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            mWidth = static_cast<uint32_t>(mode->width);
            mHeight = static_cast<uint32_t>(mode->height);
            mWindow = glfwCreateWindow(width, height, mName.c_str(), monitor, nullptr);
            glfwSetWindowAttrib(mWindow, GLFW_DECORATED, GLFW_FALSE);

            break;
        }
    }

    mContext = createInfo->vkCtx;

    {
        VkSurfaceKHR surface;
        auto res = glfwCreateWindowSurface(mContext->instance(), mWindow, nullptr, &surface);
        if (res != VK_SUCCESS)
        {
            return;
        }

        mSurface = surface;
    }
}

void Window::cleanup()
{
    mContext->instance().destroySurfaceKHR(mSurface);

    if (mWindow)
    {
        glfwDestroyWindow(mWindow);
    }
    mWindow = nullptr;
}
}  // namespace pyroc::window