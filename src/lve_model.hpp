#pragma once

#include "lve_device.hpp"
#include "lve_buffer.hpp"

//libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

//std
#include <memory>
#include <vector>

namespace lve
{

//获取CPU上创建的顶点数据，然后分配内存并将数据复制到GPU
class LveModel
{
public:
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec3 normal{};
        glm::vec2 uv{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        //定义相等比较运算符， 用与进行无序映射
        bool operator == (const Vertex& other) const
        {
            return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
        }

    };

    //存储顶点和索引数据
    struct Builder {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};

        void loadModel(const std::string &filePath);

    };

    LveModel(LveDevice &device, const LveModel::Builder &builder);
    ~LveModel();

    LveModel(const LveModel &) = delete;
    LveModel &operator=(const LveModel &) = delete;

    static std::unique_ptr<LveModel> createModelFromFile(LveDevice &device, const std::string &filePath);

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);



private:
    void createVertexBuffers(const std::vector<Vertex> &vertices);
    void createIndexBuffers(const std::vector<uint32_t> &indices);

    LveDevice& lveDevice;

    std::unique_ptr<LveBuffer> vertexBuffer;
    uint32_t vertexCount;

    bool hasIndexBuffer = false;
    std::unique_ptr<LveBuffer> indexBuffer;
    uint32_t indexCount;



};


}//namespace lve
