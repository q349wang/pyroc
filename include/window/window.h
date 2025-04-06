#pragma once

#include "backend/vulkan/api.h"

#include <string>

struct GLFWwindow;

namespace pyroc
{

namespace backend::vulkan
{
class Context;
}  // namespace backend::vulkan

namespace window
{
enum class WindowMode
{
    eWindowed = 0,
    eFullscreen = 1,
    eBorderlessWindowed = 2,
};
struct WindowCreateInfo
{
    uint32_t width = 0;
    uint32_t height = 0;
    const char* name = nullptr;
    backend::vulkan::Context* vkCtx = nullptr;
    WindowMode mode = WindowMode::eWindowed;
};

class Window
{
  public:
    void init(const WindowCreateInfo* createInfo);
    void cleanup();

    vk::SurfaceKHR surface() const { return mSurface; }
    GLFWwindow* window() const { return mWindow; }

    uint32_t width() const { return mWidth; }
    uint32_t height() const { return mHeight; }
    std::string name() const { return mName; }
    WindowMode mode() const { return mMode; }

  private:
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    std::string mName;
    WindowMode mMode = WindowMode::eWindowed;
    backend::vulkan::Context* mContext = nullptr;

    vk::SurfaceKHR mSurface = nullptr;
    GLFWwindow* mWindow = nullptr;
};
}  // namespace window
}  // namespace pyroc