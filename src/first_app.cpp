#include "first_app.hpp"

#include "lve_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "lve_buffer.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"


//lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//std
#include <array>
#include <chrono>
#include <cassert>
#include <stdexcept>
#include <memory>
#include <vector>

namespace lve
{

FirstApp::FirstApp()
{
    //globalPool 销毁时， globalDescriptorSets 对象也会一并销毁
    globalPool = LveDescriptorPool::Builder(lveDevice)
                 //set 的数量不能超过设定值
                 .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                 //一个类型的 descriptor 只有对应个，但可以自由地分到其他 set 中
                 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                 .build();
    loadGameObjects();
}

FirstApp::~FirstApp() {}

void FirstApp::run()
{

    std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<LveBuffer>(
                            lveDevice,
                            sizeof(GlobalUbo),
                            1,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            //不是coherent是因为希望选择性地刷新buffer部分内容，以免干扰可能正在渲染的前一帧
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        uboBuffers[i]->map();
    }

    auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                           .addBinding(
                               0,
                               VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                               VK_SHADER_STAGE_ALL_GRAPHICS
                           )
                           .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        //用于分配 descriptor 内存以及写入 bufferInfo
        LveDescriptorWriter(*globalSetLayout, *globalPool)
        .writeBuffer(0, &bufferInfo)
        .build(globalDescriptorSets[i]);
    }

    SimpleRenderSystem simpleRenderSystem{lveDevice,
                                          lveRenderer.getSwapChainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout()};

    PointLightSystem pointLightSystem{lveDevice,
                                      lveRenderer.getSwapChainRenderPass(),
                                      globalSetLayout->getDescriptorSetLayout()};

    LveCamera camera{};
    //    camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5f, 0.f, 1.f));
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    //用于存储camera状态
    auto viewerObject = LveGameObject::createGameObject();
    viewerObject.transform.translation.z = 0.f;
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    while(!lveWindow.shouldClose()) {
        glfwPollEvents();

        //在glfwPollEvents()之后，防止其阻塞带来的影响
        auto newTime = std::chrono::high_resolution_clock::now();
        //半秒则framTime为0.5
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        //frameTime = glm::min(frameTime, MAX_FRAME_TIME);

        cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspect = lveRenderer.getAspectRatio();
        //使得窗口大小不会拉伸原图像
        //camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

        if (auto commandBuffer = lveRenderer.beginFrame()) {
            int frameIndex = lveRenderer.getFrameIndex();

            assert(frameIndex >= 0 && frameIndex < globalDescriptorSets.size());

            FrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                camera,
                globalDescriptorSets[frameIndex],
                gameObjects
            };

            //update
            GlobalUbo ubo{};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            pointLightSystem.update(frameInfo, ubo);
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            //由于未设置coherent属性，故需要手动同步对应的数据
            uboBuffers[frameIndex]->flush();

            //render
            lveRenderer.beginSwapChainRenderPass(commandBuffer);

            simpleRenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);
            lveRenderer.endSwapChainRenderPass(commandBuffer);
            lveRenderer.endFrame();
        }
    }

    //使CPU阻塞至所有GPU操作完成
    vkDeviceWaitIdle(lveDevice.device());
}

void FirstApp::loadGameObjects()
{

    std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "models/viking_room.obj");
    auto room = LveGameObject::createGameObject();
    room.model = lveModel;
    room.transform.translation = {.0f, .0f, .0f};
    room.transform.rotation.x = glm::radians(90.0f);
    room.transform.rotation.y = glm::radians(90.0f);
    room.transform.scale = glm::vec3{1.f};
    gameObjects.emplace(room.getId(), std::move(room));

    lveModel = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
    auto floor = LveGameObject::createGameObject();
    floor.model = lveModel;
    floor.transform.translation = {.0f, .5f, 0.f};
    floor.transform.scale = glm::vec3{3.f};
    gameObjects.emplace(floor.getId(), std::move(floor));

    {

        auto pointLight = LveGameObject::makePointLight(0.2f);
        pointLight.transform.translation = {0.f, -1.5f, 0.f};
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));

        std::vector<glm::vec3> lightColors{
            {1.f, .1f, .1f},
            {.1f, .1f, 1.f},
            {.1f, 1.f, .1f},
            {1.f, 1.f, .1f},
            {.1f, 1.f, 1.f},
            {1.f, 1.f, 1.f}
        };

        for (int i = 0; i < lightColors.size(); i++) {
            auto pointLight = LveGameObject::makePointLight(.2f);
            pointLight.color = lightColors[i];
            auto rotateLight = glm::rotate(
                                   glm::mat4(1.f),
                                   (i * glm::two_pi<float>()) / lightColors.size(),
            {0.f, -1.f, 0.f});
            pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
            gameObjects.emplace(pointLight.getId(), std::move(pointLight));
        }

    }// 用于销毁pointLight
}
} //namespace lve
