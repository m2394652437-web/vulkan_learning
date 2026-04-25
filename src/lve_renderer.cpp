#include "lve_renderer.hpp"

//std
#include <array>
#include <cassert>
#include <vector>
#include <stdexcept>

namespace lve
{
LveRenderer::LveRenderer(LveWindow& window, LveDevice& device) : lveWindow{window}, lveDevice(device)
{
    recreateSwapChain();
    createCommandBuffers();
}

LveRenderer::~LveRenderer()
{
    //renderer的生命周期与app独立了
    freeCommandBuffers();
}

void LveRenderer::recreateSwapChain()
{
    auto extent = lveWindow.getExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = lveWindow.getExtent();
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(lveDevice.device());

    if (lveSwapChain == nullptr) {
        lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent);
    } else {
        std::shared_ptr<LveSwapChain> oldSwapChain = std::move(lveSwapChain);
        lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent, oldSwapChain);

        if(!oldSwapChain->compareSwapFormats(*lveSwapChain.get())) {
            throw std::runtime_error("Swap chain image(or depth format has changed!");
        }

        //由于采用的command buffer数量不依赖与swapchain大小，故可以去除
        // if (lveSwapChain->imageCount() != commandBuffers.size()){
        //   freeCommandBuffers();
        //   createCommandBuffers();
        // }

    }

}

void LveRenderer::createCommandBuffers()
{
    //此处command Buffer 的数量与swapchain 大小不一定相同
    commandBuffers.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //主命令缓冲区可以提交到队列执行，但不能被其他命令调用
    //辅助命令缓冲区则不能提交到队列执行， 可以被其他缓冲区调用
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = lveDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if(vkAllocateCommandBuffers(lveDevice.device(),&allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void LveRenderer::freeCommandBuffers()
{
    vkFreeCommandBuffers(lveDevice.device(),
                         lveDevice.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
    commandBuffers.clear();
}

VkCommandBuffer LveRenderer::beginFrame()
{
    assert(!isFrameStarted && "Cannot call beginFrame while already in progress");

    //获取接下来要渲染的帧索引，自动处理与双缓冲或三缓冲相关的CPU与GPU同步
    auto result = lveSwapChain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    isFrameStarted = true;

    auto commandBuffer = getCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    return commandBuffer;
}

void LveRenderer::endFrame()
{
    assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
    auto commandBuffer = getCurrentCommandBuffer();

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }

    //将提供的命令缓冲区提交到设备图形队列，同时处理CPU和GPU同步
    auto result = lveSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || lveWindow.wasWindowResized()) {
        lveWindow.resetWindowResizedFlag();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }

    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % LveSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void LveRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
    assert(isFrameStarted && "Cannot call beginSwapChainRenderPass if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a diferent frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = lveSwapChain->getRenderPass();
    renderPassInfo.framebuffer = lveSwapChain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = lveSwapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(lveSwapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(lveSwapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, lveSwapChain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

}

void LveRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
    assert(isFrameStarted && "Cannot call endSwapChainRenderPass if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a diferent frame");

    vkCmdEndRenderPass(commandBuffer);
}

} //namespace lve
