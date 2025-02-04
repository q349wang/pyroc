#pragma once

struct GLFWwindow;

namespace pyroc
{

namespace backend::vulkan
{
class VulkanBackend;

}
class Window
{
  public:
    void init();
    void cleanup();

    GLFWwindow* window() { return mWindow; }
    backend::vulkan::VulkanBackend* backend() { return mBackend; }

  private:
    GLFWwindow* mWindow = nullptr;
    backend::vulkan::VulkanBackend* mBackend = nullptr;
};
}  // namespace pyroc