#include <iostream>

#include "backend/vulkan/api.h"
#include "backend/vulkan/shader.h"

#include "window/window.h"

using namespace pyroc::backend::vulkan;

namespace
{
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct RenderCommand
{
    vk::CommandBuffer commandBuffer;
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    vk::Fence inFlightFence;
};

class App
{
  public:
    vk::Result createFramebuffers()
    {
        const auto swapchainImageViews = mWindow->backend()->swapchainImageViews();
        mFramebuffers.resize(swapchainImageViews.size());
        for (uint32_t i = 0; i < swapchainImageViews.size(); ++i)
        {
            vk::ImageView attachments[] = {
                swapchainImageViews[i],
            };

            const vk::FramebufferCreateInfo framebufferInfo = {
                .renderPass = mRenderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = mWindow->backend()->swapchainExtent().width,
                .height = mWindow->backend()->swapchainExtent().height,
                .layers = 1,
            };

            const auto [res, handle] = mDevice.createFramebuffer(framebufferInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }

            mFramebuffers[i] = handle;
        }

        return vk::Result::eSuccess;
    }

    void destroyFramebuffers()
    {
        for (auto framebuffer : mFramebuffers)
        {
            mDevice.destroyFramebuffer(framebuffer);
        }
    }

    void recreateSwapchain()
    {
        auto res = mDevice.waitIdle();
        if (res != vk::Result::eSuccess)
        {
            abort();
        }

        destroyFramebuffers();

        mWindow->backend()->recreateSwapchain(mWindow->window());

        res = createFramebuffers();
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
    }

    vk::Result init(pyroc::Window* window, vk::Device device)
    {
        mWindow = window;
        mDevice = device;

        mVs = createShaderFromFile(mDevice, "demos/basic/shaders/basic.vert.bin");
        mPs = createShaderFromFile(mDevice, "demos/basic/shaders/basic.frag.bin");

        const vk::PipelineShaderStageCreateInfo vsStageInfo = {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = mVs,
            .pName = "main",
        };

        const vk::PipelineShaderStageCreateInfo psStageInfo = {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = mPs,
            .pName = "main",
        };

        const vk::PipelineShaderStageCreateInfo shaderStages[] = {vsStageInfo, psStageInfo};

        std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };

        const vk::PipelineDynamicStateCreateInfo dynamicStateInfo
            = {.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
               .pDynamicStates = dynamicStates.data()};

        const vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
        };

        const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = vk::False,
        };

        const vk::PipelineViewportStateCreateInfo viewportInfo = {
            .viewportCount = 1,
            .scissorCount = 1,
        };

        const vk::PipelineRasterizationStateCreateInfo rasterInfo = {
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = vk::False,
            .lineWidth = 1.0f,
        };

        const vk::PipelineMultisampleStateCreateInfo msaaInfo = {
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = vk::False,
        };

        const vk::PipelineColorBlendAttachmentState colorAttachmentBlend = {
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                              | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };

        const vk::PipelineColorBlendStateCreateInfo colorBlendInfo = {
            .logicOpEnable = vk::False,
            .attachmentCount = 1,
            .pAttachments = &colorAttachmentBlend,
        };

        {
            vk::PipelineLayoutCreateInfo layoutInfo = {

            };
            const auto [res, handle] = device.createPipelineLayout(layoutInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mPipelineLayout = handle;
        }

        const vk::AttachmentDescription colorAttachment = {
            .format = window->backend()->swapchainFormat(),
            .samples = vk::SampleCountFlagBits::e1,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::ePresentSrcKHR,
        };

        const vk::AttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
        };

        const vk::SubpassDescription subpass = {
            .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
        };

        {
            const vk::SubpassDependency dependency = {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .srcAccessMask = {},
                .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
            };

            const vk::RenderPassCreateInfo renderPassInfo = {
                .attachmentCount = 1,
                .pAttachments = &colorAttachment,
                .subpassCount = 1,
                .pSubpasses = &subpass,
                .dependencyCount = 1,
                .pDependencies = &dependency,
            };

            const auto [res, handle] = device.createRenderPass(renderPassInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mRenderPass = handle;
        }

        {
            const vk::GraphicsPipelineCreateInfo pipelineInfo = {
                .stageCount = 2,
                .pStages = shaderStages,
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &inputAssemblyInfo,
                .pViewportState = &viewportInfo,
                .pRasterizationState = &rasterInfo,
                .pMultisampleState = &msaaInfo,
                .pDepthStencilState = nullptr,
                .pColorBlendState = &colorBlendInfo,
                .pDynamicState = &dynamicStateInfo,
                .layout = mPipelineLayout,
                .renderPass = mRenderPass,
                .subpass = 0,
            };

            const auto [res, handle] = device.createGraphicsPipeline(nullptr, pipelineInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mPipeline = handle;
        }

        {
            const auto res = createFramebuffers();
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
        }

        {
            const vk::CommandPoolCreateInfo poolInfo = {
                .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex = window->backend()->graphicsQueueIdx(),
            };

            const auto [res, handle] = device.createCommandPool(poolInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }

            mCommandPool = handle;
        }

        mRenderCommands.resize(MAX_FRAMES_IN_FLIGHT);
        {
            const vk::CommandBufferAllocateInfo allocInfo = {
                .commandPool = mCommandPool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = static_cast<uint32_t>(mRenderCommands.size()),
            };

            const auto [res, handle] = device.allocateCommandBuffers(allocInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }

            for (size_t i = 0; i < mRenderCommands.size(); ++i)
            {
                mRenderCommands[i].commandBuffer = handle[i];
            }
        }

        for (size_t i = 0; i < mRenderCommands.size(); ++i)
        {
            const vk::SemaphoreCreateInfo semaphoreInfo = {};
            const auto [res, handle] = device.createSemaphore(semaphoreInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mRenderCommands[i].imageAvailableSemaphore = handle;
        }

        for (size_t i = 0; i < mRenderCommands.size(); ++i)
        {
            const vk::SemaphoreCreateInfo semaphoreInfo = {};
            const auto [res, handle] = device.createSemaphore(semaphoreInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mRenderCommands[i].renderFinishedSemaphore = handle;
        }

        for (size_t i = 0; i < mRenderCommands.size(); ++i)
        {
            const vk::FenceCreateInfo fenceInfo = {.flags = vk::FenceCreateFlagBits::eSignaled};
            const auto [res, handle] = device.createFence(fenceInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mRenderCommands[i].inFlightFence = handle;
        }

        return vk::Result::eSuccess;
    }

    void destroy()
    {
        for (auto cmd : mRenderCommands)
        {
            mDevice.destroyFence(cmd.inFlightFence);
            mDevice.destroySemaphore(cmd.renderFinishedSemaphore);
            mDevice.destroySemaphore(cmd.imageAvailableSemaphore);
        }

        destroyFramebuffers();

        mDevice.destroyCommandPool(mCommandPool);

        mDevice.destroyPipeline(mPipeline);
        mDevice.destroyRenderPass(mRenderPass);
        mDevice.destroy(mPipelineLayout);

        mDevice.destroyShaderModule(mPs);
        mDevice.destroyShaderModule(mVs);
    }

    void drawFrame(uint32_t frameIndex)
    {
        RenderCommand& renderCommand = mRenderCommands[frameIndex];
        vk::CommandBuffer commandBuffer = renderCommand.commandBuffer;
        glfwPollEvents();
        {
            auto res = mDevice.waitForFences(renderCommand.inFlightFence, vk::True,
                                             std::numeric_limits<uint64_t>::max());
            if (res != vk::Result::eSuccess)
            {
                abort();
            }
        }

        uint32_t imageIndex;
        {
            auto res = mDevice.acquireNextImageKHR(
                mWindow->backend()->swapchain(), std::numeric_limits<uint64_t>::max(),
                renderCommand.imageAvailableSemaphore, nullptr, &imageIndex);
            switch (res)
            {
                case vk::Result::eErrorOutOfDateKHR:
                {
                    recreateSwapchain();
                    return;
                }
                case vk::Result::eSuccess:
                case vk::Result::eSuboptimalKHR:
                {
                    break;
                }
                default:
                {
                    abort();

                    break;
                }
            }
        }

        {
            const auto res = mDevice.resetFences(renderCommand.inFlightFence);
            if (res != vk::Result::eSuccess)
            {
                abort();
            }
        }
        {
            commandBuffer.reset();

            {
                const vk::CommandBufferBeginInfo beginInfo = {};
                const auto res = commandBuffer.begin(beginInfo);
                if (res != vk::Result::eSuccess)
                {
                    abort();
                }
            }

            const vk::ClearValue clearValue
                = {.color = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}};

            const vk::RenderPassBeginInfo renderPassInfo = {
                .renderPass = mRenderPass,
                .framebuffer = mFramebuffers[imageIndex],
                .renderArea = {
                    .offset = {0, 0},
                    .extent = mWindow->backend()->swapchainExtent(),
                },
                .clearValueCount = 1,
                .pClearValues = &clearValue,
            };

            commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);

            const vk::Viewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(mWindow->backend()->swapchainExtent().width),
                .height = static_cast<float>(mWindow->backend()->swapchainExtent().height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };

            commandBuffer.setViewport(0, 1, &viewport);

            const vk::Rect2D scissor{
                .offset = {0, 0},
                .extent = mWindow->backend()->swapchainExtent(),
            };

            commandBuffer.setScissor(0, 1, &scissor);

            commandBuffer.draw(3, 1, 0, 0);

            commandBuffer.endRenderPass();

            {
                const auto res = commandBuffer.end();
                if (res != vk::Result::eSuccess)
                {
                    abort();
                }
            }
        }

        {
            const vk::Semaphore waitSemaphores[] = {renderCommand.imageAvailableSemaphore};
            const vk::Semaphore signalSemaphores[] = {renderCommand.renderFinishedSemaphore};
            const vk::PipelineStageFlags waitStages[] = {
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
            };
            const vk::SubmitInfo submitInfo = {
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = waitSemaphores,
                .pWaitDstStageMask = waitStages,
                .commandBufferCount = 1,
                .pCommandBuffers = &commandBuffer,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = signalSemaphores,
            };

            vk::Queue graphicsQueue = mWindow->backend()->graphicsQueue();

            const auto res = graphicsQueue.submit(submitInfo, renderCommand.inFlightFence);
            if (res != vk::Result::eSuccess)
            {
                abort();
            }
        }

        {
            const vk::Semaphore waitSemaphores[] = {renderCommand.renderFinishedSemaphore};

            const vk::SwapchainKHR swapchains[] = {mWindow->backend()->swapchain()};
            const vk::PresentInfoKHR presentInfo = {
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = waitSemaphores,
                .swapchainCount = 1,
                .pSwapchains = swapchains,
                .pImageIndices = &imageIndex,
            };

            vk::Queue presentQueue = mWindow->backend()->presentQueue();

            const auto res = presentQueue.presentKHR(presentInfo);
            switch (res)
            {
                case vk::Result::eSuboptimalKHR:
                case vk::Result::eErrorOutOfDateKHR:
                {
                    recreateSwapchain();
                    break;
                }
                case vk::Result::eSuccess:
                {
                    break;
                }
                default:
                {
                    abort();

                    break;
                }
            }
        }
    }

  private:
    pyroc::Window* mWindow;
    vk::Device mDevice;

    vk::ShaderModule mVs;
    vk::ShaderModule mPs;

    vk::PipelineLayout mPipelineLayout;
    vk::RenderPass mRenderPass;
    vk::Pipeline mPipeline;

    vk::CommandPool mCommandPool;

    std::vector<vk::Framebuffer> mFramebuffers;

    std::vector<RenderCommand> mRenderCommands;
};

}  // namespace

int main(void)
{
    pyroc::Window window;
    window.init();
    vk::Device device = window.backend()->device();

    App app;
    {
        const auto res = app.init(&window, device);
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
    }

    glfwSetWindowUserPointer(window.window(), &app);
    glfwSetFramebufferSizeCallback(
        window.window(),
        [](GLFWwindow* glfwWindow, int, int)
        {
            App* pApp = static_cast<App*>(glfwGetWindowUserPointer(glfwWindow));
            pApp->recreateSwapchain();
        });

    uint32_t currentFrame = 0;
    while (!glfwWindowShouldClose(window.window()))
    {
        app.drawFrame(currentFrame);
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    {
        const auto res = device.waitIdle();
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
    }

    window.cleanup();
    return 0;
}