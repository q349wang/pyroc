#include <iostream>

#include "shaders/basic.h"

#include "pyroc.h"

using namespace pyroc::backend::vulkan;

using namespace pyroc::math;

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

struct Vertex
{
    vec3 pos;
    vec3 colour;
};

class App
{
  public:
    vk::Result createFramebuffers()
    {
        const auto swapchainImageViews = mSurface.swapchainImageViews;
        mFramebuffers.resize(swapchainImageViews.size());
        for (uint32_t i = 0; i < swapchainImageViews.size(); ++i)
        {
            if (!swapchainImageViews[i])
            {
                mFramebuffers[i] = nullptr;
                continue;
            }

            vk::ImageView attachments[] = {
                swapchainImageViews[i],
            };

            const vk::FramebufferCreateInfo framebufferInfo = {
                .renderPass = mRenderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = mSurface.extent.width,
                .height = mSurface.extent.height,
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
            if (framebuffer == nullptr)
            {
                continue;
            }
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

        int32_t width, height;
        glfwGetFramebufferSize(mWindow->window(), &width, &height);
        std::cout << "Recreating swapchain with size: " << width << "x" << height << std::endl;

        if (width == 0 || height == 0)
        {
            width = static_cast<int32_t>(mWindow->width());
            height = static_cast<int32_t>(mWindow->height());
        }

        const SurfaceCreateInfo recreateInfo = {
            .surface = nullptr,
            .extent = vk::Extent2D{
                .width = static_cast<uint32_t>(width),
                .height =  static_cast<uint32_t>(height),
            },
        };

        res = recreateSurface(mCtx, &recreateInfo, mSurface);

        res = createFramebuffers();
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
    }

    vk::Result init(Context* ctx, pyroc::window::Window* window)
    {
        mCtx = ctx;
        mWindow = window;
        mDevice = ctx->device();

        {
            const SurfaceCreateInfo surfaceCreateInfo = {.surface = mWindow->surface(),
                                                         .extent = vk::Extent2D{
                                                             .width = mWindow->width(),
                                                             .height = mWindow->height(),
                                                         }};

            const auto res = createSurface(mCtx, &surfaceCreateInfo, mSurface);

            if (res != vk::Result::eSuccess)
            {
                std::cerr << "Failed to create surface: " << vk::to_string(res) << std::endl;
                return res;
            }
        }

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

        const vk::VertexInputBindingDescription vertexBindingDescription = {
            .binding = IN_BINDING_VERTEX,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex,
        };

        const vk::VertexInputAttributeDescription attributeDescriptions[2]
            = {{
                   .location = IN_VERTEX_POSITION,
                   .binding = IN_BINDING_VERTEX,
                   .format = vk::Format::eR32G32Sfloat,
                   .offset = offsetof(Vertex, pos),
               },
               {.location = IN_VERTEX_COLOUR,
                .binding = IN_BINDING_VERTEX,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Vertex, colour)}};

        const vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexBindingDescription,
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions = attributeDescriptions,
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
            .cullMode = vk::CullModeFlagBits::eNone,
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
            vk::PushConstantRange pushConstantRange = {
                .stageFlags = vk::ShaderStageFlagBits::eVertex,
                .offset = 0,
                .size = sizeof(PushConstants),
            };

            vk::PipelineLayoutCreateInfo layoutInfo = {
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &pushConstantRange,
            };

            const auto [res, handle] = mDevice.createPipelineLayout(layoutInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mPipelineLayout = handle;
        }

        const vk::AttachmentDescription colorAttachment = {
            .format = mSurface.format,
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

            const auto [res, handle] = mDevice.createRenderPass(renderPassInfo);
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

            const auto [res, handle] = mDevice.createGraphicsPipeline(nullptr, pipelineInfo);
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
                .queueFamilyIndex = mCtx->graphicsQueueIdx(),
            };

            const auto [res, handle] = mDevice.createCommandPool(poolInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }

            mCommandPool = handle;
        }

        mUpdateCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        {
            const vk::CommandBufferAllocateInfo allocInfo = {
                .commandPool = mCommandPool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = static_cast<uint32_t>(mUpdateCommandBuffers.size()),
            };

            const auto [res, handle] = mDevice.allocateCommandBuffers(allocInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }

            for (size_t i = 0; i < mUpdateCommandBuffers.size(); ++i)
            {
                mUpdateCommandBuffers[i] = handle[i];
            }
        }

        mRenderCommands.resize(MAX_FRAMES_IN_FLIGHT);
        {
            const vk::CommandBufferAllocateInfo allocInfo = {
                .commandPool = mCommandPool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = static_cast<uint32_t>(mRenderCommands.size()),
            };

            const auto [res, handle] = mDevice.allocateCommandBuffers(allocInfo);
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
            const auto [res, handle] = mDevice.createSemaphore(semaphoreInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mRenderCommands[i].imageAvailableSemaphore = handle;
        }

        for (size_t i = 0; i < mRenderCommands.size(); ++i)
        {
            const vk::SemaphoreCreateInfo semaphoreInfo = {};
            const auto [res, handle] = mDevice.createSemaphore(semaphoreInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mRenderCommands[i].renderFinishedSemaphore = handle;
        }

        for (size_t i = 0; i < mRenderCommands.size(); ++i)
        {
            const vk::FenceCreateInfo fenceInfo = {.flags = vk::FenceCreateFlagBits::eSignaled};
            const auto [res, handle] = mDevice.createFence(fenceInfo);
            if (res != vk::Result::eSuccess)
            {
                return res;
            }
            mRenderCommands[i].inFlightFence = handle;
        }

        {
            Vertex verts[] = {
                {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}},  // 0
                {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},   // 2
                {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
                {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},   // 4
                {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
                {{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},    // 6
                {{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
            };

            {
                const auto res = createBuffer(
                    mCtx, sizeof(verts),
                    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                    vk::MemoryPropertyFlagBits::eDeviceLocal, mVertexBuffer);
                if (res != vk::Result::eSuccess)
                {
                    return res;
                }
            }

            {
                const auto res = copyBufferBlocking(mCtx, mUpdateCommandBuffers[0], sizeof(verts),
                                                    verts, 0, mVertexBuffer);
                if (res != vk::Result::eSuccess)
                {
                    return res;
                }
            }
        }

        {
            // clang-format off
            uint16_t indices[] =
            {
                0, 2, 1, 
                1, 2, 3,
                4, 6, 0,
                0, 6, 2,
                5, 7, 4,
                4, 7, 6,
                1, 3, 5,
                5, 3, 7,
                6, 7, 2,
                2, 7, 3,
                4, 0, 5,
                5, 0, 1,
            };
            // clang-format on
            {
                const auto res = createBuffer(
                    mCtx, sizeof(indices),
                    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                    vk::MemoryPropertyFlagBits::eDeviceLocal, mIndexBuffer);
                if (res != vk::Result::eSuccess)
                {
                    return res;
                }
            }

            {
                const auto res = copyBufferBlocking(mCtx, mUpdateCommandBuffers[0], sizeof(indices),
                                                    indices, 0, mIndexBuffer);
                if (res != vk::Result::eSuccess)
                {
                    return res;
                }
            }
        }

        {
            mCamera = {
                .eye = vec3{0.0f, 0.0f, 10.0f},
                .center = vec3{0.0f, 0.0f, 0.0f},
                .up = vec3{0.0f, 1.0f, 0.0f},
                .fovY = 45.0f,
                .aspect = static_cast<float>(mSurface.extent.width)
                          / static_cast<float>(mSurface.extent.height),
                .nearPlane = 0.1f,
                .farPlane = 100.0f,
            };

            mViewMatrix = mCamera.viewMatrix();
            mProjMatrix = mCamera.projectionMatrix();
        }

        return vk::Result::eSuccess;
    }

    void destroy()
    {
        destroyBuffer(mCtx, mVertexBuffer);
        destroyBuffer(mCtx, mIndexBuffer);
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

        destroySurface(mCtx, mSurface);
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
            if (mSurface.swapchain == nullptr)
            {
                return;
            }
            auto res = mDevice.acquireNextImageKHR(
                mSurface.swapchain, std::numeric_limits<uint64_t>::max(),
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

        if (!mFramebuffers[imageIndex])
        {
            return;
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
                    .extent = mSurface.extent,
                },
                .clearValueCount = 1,
                .pClearValues = &clearValue,
            };

            commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);

            vk::Buffer vertexBuffers[] = {mVertexBuffer.buffer};
            vk::DeviceSize offsets[] = {0};
            commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
            commandBuffer.bindIndexBuffer(mIndexBuffer.buffer, 0, vk::IndexType::eUint16);

            const vk::Viewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(mSurface.extent.width),
                .height = static_cast<float>(mSurface.extent.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };

            commandBuffer.setViewport(0, 1, &viewport);

            const vk::Rect2D scissor{
                .offset = {0, 0},
                .extent = mSurface.extent,
            };

            commandBuffer.setScissor(0, 1, &scissor);

            PushConstants pc = {
                .model = mModelMatrix,
                .view = mViewMatrix,
                .projection = mProjMatrix,
            };
            commandBuffer.pushConstants(mPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0,
                                        sizeof(PushConstants), &pc);

            commandBuffer.drawIndexed(12, 1, 0, 3, 0);

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

            vk::Queue graphicsQueue = mCtx->graphicsQueue();

            const auto res = graphicsQueue.submit(submitInfo, renderCommand.inFlightFence);
            if (res != vk::Result::eSuccess)
            {
                abort();
            }
        }

        {
            const vk::Semaphore waitSemaphores[] = {renderCommand.renderFinishedSemaphore};

            const vk::SwapchainKHR swapchains[] = {mSurface.swapchain};
            const vk::PresentInfoKHR presentInfo = {
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = waitSemaphores,
                .swapchainCount = 1,
                .pSwapchains = swapchains,
                .pImageIndices = &imageIndex,
            };

            vk::Queue presentQueue = mCtx->presentQueue();

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
    Context* mCtx;

    pyroc::window::Window* mWindow;

    vk::Device mDevice;

    vk::ShaderModule mVs;
    vk::ShaderModule mPs;

    vk::PipelineLayout mPipelineLayout;
    vk::RenderPass mRenderPass;
    vk::Pipeline mPipeline;

    vk::CommandPool mCommandPool;

    std::vector<vk::CommandBuffer> mUpdateCommandBuffers;

    Surface mSurface;
    std::vector<vk::Framebuffer> mFramebuffers;

    std::vector<RenderCommand> mRenderCommands;

    Buffer mVertexBuffer;
    Buffer mIndexBuffer;

    pyroc::core::Camera mCamera;
    mat4 mModelMatrix = {
        vec4{1.0f, 0.0f, 0.0f, 0.0f},
        vec4{0.0f, 1.0f, 0.0f, 0.0f},
        vec4{0.0f, 0.0f, 1.0f, 0.0f},
        vec4{0.0f, 0.0f, 0.0f, 1.0f},
    };
    mat4 mViewMatrix = mat4::identity();
    mat4 mProjMatrix = mat4::identity();
};

}  // namespace

int main(void)
{
    Context ctx;
    {
        vk::Result res = ctx.init();
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
    }

    const pyroc::window::WindowCreateInfo windowCreateInfo = {
        .width = 800,
        .height = 600,
        .name = "Pyroc Basic Demo",
        .vkCtx = &ctx,
        .mode = pyroc::window::WindowMode::eWindowed,  // Change to eFullscreen or
                                                       // eBorderlessWindowed if needed
    };

    pyroc::window::Window window;
    window.init(&windowCreateInfo);

    vk::Device device = ctx.device();

    App app;
    {
        const auto res = app.init(&ctx, &window);
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
            std::cout << "Framebuffer size changed, recreating swapchain...\n" << std::endl;
            pApp->recreateSwapchain();
        });

    glfwSetWindowSizeCallback(
        window.window(),
        [](GLFWwindow* glfwWindow, int, int)
        {
            App* pApp = static_cast<App*>(glfwGetWindowUserPointer(glfwWindow));

            std::cout << "Window size changed, recreating swapchain...\n" << std::endl;
            pApp->recreateSwapchain();
        });

    glfwSetWindowFocusCallback(
        window.window(),
        [](GLFWwindow* glfwWindow, int focused)
        {
            App* pApp = static_cast<App*>(glfwGetWindowUserPointer(glfwWindow));
            if (focused)
            {
                std::cout << "Window focused, recreating swapchain...\n" << std::endl;
                pApp->recreateSwapchain();
            }
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

    app.destroy();
    window.cleanup();
    return 0;
}
