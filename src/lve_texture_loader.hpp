#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "lve_device.hpp"

namespace lve
{

class LveTextureLoader
{
public:
    LveTextureLoader(LveDevice& lveDevice);

    void createTextureImage();
    void createImage(int width,
                     int height,
                     VkImage& image,
                     VkDeviceMemory& imageMemory,
                     VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
                     VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
                     VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                    );

private:
    LveDevice& lveDevice;
    VkDevice device;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;

};

}// namespace lve

