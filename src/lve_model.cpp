#include "lve_model.hpp"
#include "lve_utils.hpp"

//libs
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

//std
#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace std
{

//告诉无序映射该如何计算Vertex的hash
template<>
struct hash<lve::LveModel::Vertex> {
    size_t operator()(lve::LveModel::Vertex const& vertex) const
    {
        size_t seed = 0;
        lve::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
        return seed;
    }
};

}// namespace std

namespace lve
{
LveModel::LveModel(LveDevice &device, const LveModel::Builder &builder): lveDevice{device}
{
    createVertexBuffers(builder.vertices);
    createIndexBuffers(builder.indices);
}

LveModel::~LveModel()
{
    //buffer的内存已经在lve_buffer文件处理
}

std::unique_ptr<LveModel> LveModel:: createModelFromFile(LveDevice &device, const std::string &filePath)
{

    Builder builder{};
    builder.loadModel(filePath);
    std::cout << "Vertex count:" << builder.vertices.size() << "\n";
    return std::make_unique<LveModel>(device, builder);

}

void LveModel:: createVertexBuffers(const std::vector<Vertex> &vertices)
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Vertex count must be at least 3");

    //顶点缓冲区存储模型所有的顶点所需要的总字节数
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
    uint32_t vertexSize = sizeof(vertices[0]);

    //staging buffer是为了将数据从CPU拷贝到GPU以获得更快的计算速度
    LveBuffer stagingBuffer{
        lveDevice,
        vertexSize,
        vertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

    };

    //将GPU内存映射到CPU，使CPU可以读写数据
    //创建一个映射到设备（GPU）内存的主机（CPU）内存区域
    //并将数据设置为指向映射内存范围的开头
    //   CPU       |        GPU
    //void* data  -->  vertex buffer memory
    //获取顶点数据复制到主机映射内存区域（void* data）
    //由于有VK_MEMORY_PROPERTY_HOST_COHERENT_BIT，主机内存将自动刷新以更新设备内存
    //数据传输到GPU上后可unmap， 但此处清理staging buffer时候会自动处理这部分内存
    stagingBuffer.map();
    stagingBuffer.writeToBuffer((void*)vertices.data());

    vertexBuffer = std::make_unique<LveBuffer>(
                       lveDevice,
                       vertexSize,
                       vertexCount,
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       //每当HOST数据变化时，传递数据给DEVICE
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //将staging buffer的数据复制到local
    lveDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);

    //此时staging buffer是栈变量，会在函数结束后自动清理
}


void LveModel:: createIndexBuffers(const std::vector<uint32_t> &indices)
{
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;

    if(!hasIndexBuffer) {
        return;
    }

    //顶点缓冲区存储模型所有的顶点所需要的总字节数
    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
    uint32_t indexSize = sizeof(indices[0]);

    LveBuffer stagingBuffer{
        lveDevice,
        indexSize,
        indexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    stagingBuffer.map();
    stagingBuffer.writeToBuffer((void*) indices.data());

    indexBuffer = std::make_unique<LveBuffer>(
                      lveDevice,
                      indexSize,
                      indexCount,
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      //这块内存应该分配在 GPU 的专用显存（VRAM）中
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    lveDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);

}

void LveModel::draw(VkCommandBuffer commandBuffer)
{

    if (hasIndexBuffer) {
        vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
}

void LveModel::bind(VkCommandBuffer commandBuffer)
{
    VkBuffer buffers[] = {vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};
    //绑定顶点缓冲区的信息记录到命令缓冲区
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    if (hasIndexBuffer) {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

}
std::vector<VkVertexInputBindingDescription> LveModel::Vertex::getBindingDescriptions()
{
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> LveModel::Vertex::getAttributeDescriptions()
{

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    //location, binding, format, offset
    //与顶点着色器中设置的location匹配
    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
    attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
    attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

    return attributeDescriptions;

}

void LveModel::Builder::loadModel(const std::string &filePath)
{
    std::string Path = ENGINE_DIR + filePath;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, Path.c_str())) {

        throw std::runtime_error(warn + err);

    }

    vertices.clear();
    indices.clear();

    //将跟踪已添加到builer.vertices中的顶点，并存储顶点最初添加的位置
    //需要知道如何计算hash以及如何比较相等
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {

            Vertex vertex{};

            //如果index < 0 则说明无index
            if (index.vertex_index >= 0) {
                vertex.position = {
                    //每个顶点有三个属性
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
                };

                vertex.color = {
                    //每个顶点有三个属性
                    attrib.colors[3 * index.vertex_index + 0],
                    attrib.colors[3 * index.vertex_index + 1],
                    attrib.colors[3 * index.vertex_index + 2],

                };

                //如果index < 0 则说明无index
                if (index.normal_index >= 0) {
                    vertex.normal = {
                        //normal有三个属性
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],

                    };
                }

                //如果index < 0 则说明无index
                if (index.vertex_index >= 0) {
                    vertex.uv = {
                        //uv2个属性
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1],

                    };
                }

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }

        }
    }

}

}//namespace lve
