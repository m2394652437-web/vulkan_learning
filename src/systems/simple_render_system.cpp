#include "simple_render_system.hpp"

//lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//std
#include <array>
#include <cassert>
#include <stdexcept>
#include <memory>
#include <vector>

namespace lve
{

struct SimplePushConstantData {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};

};

SimpleRenderSystem::SimpleRenderSystem(LveDevice& device,
                                       VkRenderPass renderPass,
                                       VkDescriptorSetLayout globalSetLayout) : lveDevice{device}
{
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem()
{
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
    VkPushConstantRange pushConstantRange{};
    //能在顶点和片段着色器中都能访问推送常量数据
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};


    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    //将顶点数据以外的数据传给顶点着色器和片段着色器
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    //高效地相着色器程序发送少量数据的方法
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass)
{
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    LvePipeline::defaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    lvePipeline = std::make_unique<LvePipeline>(
                      lveDevice,
                      "shaders/simple_shader.vert.spv",
                      "shaders/simple_shader.frag.spv",
                      pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo)
{

    //render
    lvePipeline->bind(frameInfo.commandBuffer);


    //只需绑定一次则所有的gameObj都可以使用全局UBO的值
    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        //第一个 descriptor 的 count
        0,
        //绑定 descriptor 时可以一次绑定多个 set， 但是必须指定起始set
        //起始 set 之后的每个集合必须重新绑定
        //所以更加频繁共享的set应当占据更前的集合编号
        1,
        &frameInfo.globalDescriptorSet,
        0,
        nullptr);


    for (auto& kv : frameInfo.gameObjects) {
        auto& obj = kv.second;
	//无模型的obj则跳过渲染
        if (obj.model == nullptr) continue;
        SimplePushConstantData push{};
        push.modelMatrix = obj.transform.mat4();
        push.normalMatrix = obj.transform.normalMatrix();

        vkCmdPushConstants(frameInfo.commandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(SimplePushConstantData),
                           &push);

        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }
}

} //namespace lve
