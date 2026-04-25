#pragma once

/*
封装所有帧相关数据
*/

#include "lve_camera.hpp"
#include "lve_game_object.hpp"


//lib
#include <vulkan/vulkan.h>

namespace lve
{

#define MAX_LIGHTS 10

struct PointLight {
    glm::vec4 position{}; //w为了对齐
    glm::vec4 color{};
    //对齐：
    //Position  |    Color
    //x  y  z   |    r   g   b   i
    //4B 8B 12B 16B  20B 24B 28B 32B
    //故vec4不需要额外对其
};

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::mat4 inverseView{1.f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};
    //alignas(16) glm::vec3 lightDirection = glm::normalize(glm::vec3{1.f, -3.f, -1.f});
    PointLight pointLights[MAX_LIGHTS];
    int numLights;
};

struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    LveCamera &camera;
    VkDescriptorSet globalDescriptorSet;
    LveGameObject::Map& gameObjects;
};

}// namespace lve
