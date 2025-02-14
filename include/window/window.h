#pragma once

struct GLFWwindow;

namespace pyroc
{

namespace backend::vulkan
{
class Context;

}
class Window
{
  public:
    void init();
    void cleanup();

    GLFWwindow* window() { return mWindow; }
    backend::vulkan::Context* ctx() { return mContext; }

  private:
    GLFWwindow* mWindow = nullptr;
    backend::vulkan::Context* mContext = nullptr;
};
}  // namespace pyroc