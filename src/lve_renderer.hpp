/*
  管理swap chain 和 渲染过程

 */
#pragma once

#include "lve_device.hpp"
#include "lve_model.hpp"
#include "lve_swap_chain.hpp"
#include "lve_window.hpp"

//std
#include <memory>
#include <vector>
#include <cassert>

namespace lve
{

class LveRenderer
{
public:

    LveRenderer(LveWindow& window, LveDevice& device);
    ~LveRenderer();

    LveRenderer(const LveRenderer &) = delete;
    LveRenderer &operator=(const LveRenderer &) = delete;


    VkRenderPass getSwapChainRenderPass() const
    {
        return lveSwapChain->getRenderPass();
    }

    float getAspectRatio() const
    {
        return lveSwapChain->extentAspectRatio();
    }

    bool isFrameInProgress() const
    {
        return isFrameStarted;
    }

    VkCommandBuffer getCurrentCommandBuffer() const
    {
        assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
        return commandBuffers[currentFrameIndex];
    }

    int getFrameIndex() const
    {
        assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
        return currentFrameIndex;
    }

    VkCommandBuffer beginFrame();
    void endFrame();
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();

    LveWindow& lveWindow;
    LveDevice& lveDevice;
    //用unique_ptr可以更轻松创建一个具有更新后的宽度和高度的新交换链，只需要直接构造一个新对象即可
    std::unique_ptr<LveSwapChain> lveSwapChain;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentImageIndex;
    int currentFrameIndex{0};
    bool isFrameStarted{false};

};
} //namespace lve
