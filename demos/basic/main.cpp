#include <iostream>

#include "backend/vulkan/api.h"
#include "backend/vulkan/shader.h"

#include "window/window.h"

using namespace pyroc::backend::vulkan;

int main(void)
{
    pyroc::Window window;
    window.init();
    vk::Device device = window.backend()->device();

    vk::ShaderModule vs = createShaderFromFile(device, "demos/basic/shaders/basic.vert.bin");
    vk::ShaderModule ps = createShaderFromFile(device, "demos/basic/shaders/basic.frag.bin");

    const vk::PipelineShaderStageCreateInfo vsStageInfo = {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = vs,
        .pName = "main",
    };

    const vk::PipelineShaderStageCreateInfo psStageInfo = {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = ps,
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

    vk::PipelineLayout pipelineLayout;
    {
        vk::PipelineLayoutCreateInfo layoutInfo = {

        };
        const auto [res, handle] = device.createPipelineLayout(layoutInfo);
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
        pipelineLayout = handle;
    }

    const vk::AttachmentDescription colorAttachment = {
        .format = window.backend()->swapchainFormat(),
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

    vk::RenderPass renderPass;
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
            abort();
        }
        renderPass = handle;
    }

    vk::Pipeline pipeline;
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
            .layout = pipelineLayout,
            .renderPass = renderPass,
            .subpass = 0,
        };

        const auto [res, handle] = device.createGraphicsPipeline(nullptr, pipelineInfo);
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
        pipeline = handle;
    }

    const auto swapchainImageViews = window.backend()->swapchainImageViews();
    std::vector<vk::Framebuffer> framebuffers(swapchainImageViews.size());
    for (uint32_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        vk::ImageView attachments[] = {
            swapchainImageViews[i],
        };

        const vk::FramebufferCreateInfo framebufferInfo = {
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = window.backend()->swapchainExtent().width,
            .height = window.backend()->swapchainExtent().height,
            .layers = 1,
        };

        const auto [res, handle] = device.createFramebuffer(framebufferInfo);
        if (res != vk::Result::eSuccess)
        {
            abort();
        }

        framebuffers[i] = handle;
    }

    vk::CommandPool commandPool;
    {
        const vk::CommandPoolCreateInfo poolInfo = {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = window.backend()->graphicsQueueIdx(),
        };

        const auto [res, handle] = device.createCommandPool(poolInfo);
        if (res != vk::Result::eSuccess)
        {
            abort();
        }

        commandPool = handle;
    }

    vk::CommandBuffer commandBuffer;
    {
        const vk::CommandBufferAllocateInfo allocInfo = {
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };

        const auto [res, handle] = device.allocateCommandBuffers(allocInfo);
        if (res != vk::Result::eSuccess)
        {
            abort();
        }

        commandBuffer = handle[0];
    }

    vk::Semaphore imageAvailableSemaphore;
    {
        const vk::SemaphoreCreateInfo semaphoreInfo = {};
        const auto [res, handle] = device.createSemaphore(semaphoreInfo);
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
        imageAvailableSemaphore = handle;
    }

    vk::Semaphore renderFinishedSemaphore;
    {
        const vk::SemaphoreCreateInfo semaphoreInfo = {};
        const auto [res, handle] = device.createSemaphore(semaphoreInfo);
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
        renderFinishedSemaphore = handle;
    }

    vk::Fence inFlightFence;
    {
        const vk::FenceCreateInfo fenceInfo = {.flags = vk::FenceCreateFlagBits::eSignaled};
        const auto [res, handle] = device.createFence(fenceInfo);
        if (res != vk::Result::eSuccess)
        {
            abort();
        }
        inFlightFence = handle;
    }

    while (!glfwWindowShouldClose(window.window()))
    {
        glfwPollEvents();
        {
            auto res = device.waitForFences(inFlightFence, vk::True,
                                            std::numeric_limits<uint64_t>::max());
            if (res != vk::Result::eSuccess)
            {
                abort();
            }

            res = device.resetFences(inFlightFence);
            if (res != vk::Result::eSuccess)
            {
                abort();
            }
        }

        uint32_t imageIndex;
        {
            auto res = device.acquireNextImageKHR(window.backend()->swapchain(),
                                                  std::numeric_limits<uint64_t>::max(),
                                                  imageAvailableSemaphore, nullptr, &imageIndex);
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
                .renderPass = renderPass,
                .framebuffer = framebuffers[imageIndex],
                .renderArea = {
                    .offset = {0, 0},
                    .extent = window.backend()->swapchainExtent(),
                },
                .clearValueCount = 1,
                .pClearValues = &clearValue,
            };

            commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

            const vk::Viewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(window.backend()->swapchainExtent().width),
                .height = static_cast<float>(window.backend()->swapchainExtent().height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };

            commandBuffer.setViewport(0, 1, &viewport);

            const vk::Rect2D scissor{
                .offset = {0, 0},
                .extent = window.backend()->swapchainExtent(),
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
            const vk::Semaphore waitSemaphores[] = {imageAvailableSemaphore};
            const vk::Semaphore signalSemaphores[] = {renderFinishedSemaphore};
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

            vk::Queue graphicsQueue = window.backend()->graphicsQueue();

            const auto res = graphicsQueue.submit(submitInfo, inFlightFence);
            if (res != vk::Result::eSuccess)
            {
                abort();
            }
        }

        {
            const vk::Semaphore waitSemaphores[] = {renderFinishedSemaphore};

            const vk::SwapchainKHR swapchains[] = {window.backend()->swapchain()};
            const vk::PresentInfoKHR presentInfo = {
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = waitSemaphores,
                .swapchainCount = 1,
                .pSwapchains = swapchains,
                .pImageIndices = &imageIndex,
            };

            vk::Queue presentQueue = window.backend()->presentQueue();

            const auto res = presentQueue.presentKHR(presentInfo);
            if (res != vk::Result::eSuccess)
            {
                abort();
            }
        }
    }

    device.destroyFence(inFlightFence);
    device.destroySemaphore(renderFinishedSemaphore);
    device.destroySemaphore(imageAvailableSemaphore);

    for (auto framebuffer : framebuffers)
    {
        device.destroyFramebuffer(framebuffer);
    }

    device.destroyCommandPool(commandPool);

    device.destroyPipeline(pipeline);
    device.destroyRenderPass(renderPass);
    device.destroy(pipelineLayout);

    device.destroyShaderModule(ps);
    device.destroyShaderModule(vs);

    window.cleanup();
    return 0;
}