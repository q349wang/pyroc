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
    void loop();
    void cleanup();

  private:
    GLFWwindow* mWindow = nullptr;
    backend::vulkan::VulkanBackend* mBackend = nullptr;
};
}  // namespace pyroc