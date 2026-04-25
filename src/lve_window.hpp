
#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#ifndef LVE_WINDOW_H
#define LVE_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>//用于创建新的窗口
#include <string>

namespace lve
{

class LveWindow
{
public:
    LveWindow (int w, int h, std::string name);
    ~LveWindow();

    // 禁用拷贝构造函数
    LveWindow(const LveWindow&) = delete;
    // 禁用拷贝赋值运算符
    LveWindow &operator=(const LveWindow &) = delete;

    bool shouldClose()
    {
        return glfwWindowShouldClose(window);
    }
    VkExtent2D getExtent()
    {
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    bool wasWindowResized()
    {
        return framebufferResized;
    }

    void resetWindowResizedFlag()
    {
        framebufferResized = false;
    }

    GLFWwindow* getGLFWwindow() const
    {
        return window;
    }
 
    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

private:
    static void framebufferResizedCallback(GLFWwindow *window, int width, int height);
    void initWindow();

    int width;
    int height;
    bool framebufferResized = false;

    std::string windowName;
    GLFWwindow *window;

};

}//namespace lve

#endif /* LVE_WINDOW_H */
