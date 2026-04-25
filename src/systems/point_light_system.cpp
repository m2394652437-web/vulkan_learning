#include "point_light_system.hpp"

//lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//std
#include <array>
#include <cassert>
#include <stdexcept>
#include <map>

namespace lve
{

struct PointLightPushConstants {
    glm::vec4 position{};
    glm::vec4 color{};
    float radius;
};

PointLightSystem::PointLightSystem(LveDevice& device,
                                   VkRenderPass renderPass,
                                   VkDescriptorSetLayout globalSetLayout) : lveDevice{device}
{
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

PointLightSystem::~PointLightSystem()
{
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
}

void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
    VkPushConstantRange pushConstantRange{};
    //能在顶点和片段着色器中都能访问推送常量数据
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PointLightPushConstants);

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

void PointLightSystem::createPipeline(VkRenderPass renderPass)
{
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
    LvePipeline::enableAlphaBlending(pipelineConfig);
    pipelineConfig.attributeDescriptions.clear();
    pipelineConfig.bindingDescriptions.clear();
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    lvePipeline = std::make_unique<LvePipeline>(
                      lveDevice,
                      "shaders/point_light.vert.spv",
                      "shaders/point_light.frag.spv",
                      pipelineConfig);
}

//将点光源信息同步至ubo
void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo)
{

    auto rotateLight = glm::rotate(
                           glm::mat4(1.f),
                           frameInfo.frameTime,
    {0.f, -1.f, 0.f});

    int lightIndex = 0;
    for (auto& kv: frameInfo.gameObjects) {
        auto& obj = kv.second;
        if (obj.pointLight == nullptr) continue;

        assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");

        //update light position
        obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

        //copy light to ubo
        ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
        ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
        lightIndex ++;
    }

    ubo.numLights = lightIndex;
}

void PointLightSystem::render(FrameInfo& frameInfo)
{

    std::multimap<float, LveGameObject::id_t> sorted;
    for (auto& kv: frameInfo.gameObjects) {
        auto& obj = kv.second;
        if (obj.pointLight == nullptr) continue;

        //distance
        auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
        float disSquared = glm::dot(offset, offset);
	sorted.insert({disSquared, obj.getId()});
    }

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

    //iterate through sorted lights in reverse order
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
        auto& obj = frameInfo.gameObjects.at(it->second);

        if (obj.pointLight == nullptr) continue;

        PointLightPushConstants push{};
        push.position = glm::vec4(obj.transform.translation, 1.f);
        push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
        push.radius = obj.transform.scale.x;

        vkCmdPushConstants(frameInfo.commandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(PointLightPushConstants),
                           &push);
        vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }

    // for (auto& kv : frameInfo.gameObjects) {
    //     auto& obj = kv.second;
    //     //无模型的obj则跳过渲染
    //     if (obj.model == nullptr) continue;
    //     SimplePushConstantData push{};
    //     push.modelMatrix = obj.transform.mat4();
    //     push.normalMatrix = obj.transform.normalMatrix();

    //     vkCmdPushConstants(frameInfo.commandBuffer,
    //                        pipelineLayout,
    //                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    //                        0,
    //                        sizeof(SimplePushConstantData),
    //                        &push);

    //     obj.model->bind(frameInfo.commandBuffer);
    //     obj.model->draw(frameInfo.commandBuffer);
    // }

}

} //namespace lve
