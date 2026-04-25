#include "lve_window.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace lve
{
LveWindow::LveWindow(int w, int h, std::string name): width{w}, height{h}, windowName(name)
{
    initWindow();
}

LveWindow::~LveWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void LveWindow::initWindow()
{
    glfwInit();
    //通过NO_API来阻止GLFW用OpenGL来初始化窗口
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //是否阻止窗口被创建后大小改变
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    //4th 是否创建全屏的窗口
    //5th 与OpenGL相关
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
    
}

void LveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface)
{
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface");
}

  void LveWindow::framebufferResizedCallback(GLFWwindow *window, int width, int height){
    auto lveWindow = reinterpret_cast<LveWindow *>(glfwGetWindowUserPointer(window));
    lveWindow->framebufferResized = true;
    lveWindow->width = width;
    lveWindow->height = height;
  }
  
}//namespace lve
