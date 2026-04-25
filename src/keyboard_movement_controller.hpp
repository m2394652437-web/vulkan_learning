#pragma once
#ifndef KEYBOARD_MOVEMENT_CONTROLLER_H
#define KEYBOARD_MOVEMENT_CONTROLLER_H

#include "lve_game_object.hpp"
#include "lve_window.hpp"

namespace lve
{

class KeyboardMovementController
{

public:

    struct KeyMappings {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int moveUp = GLFW_KEY_LEFT_SHIFT;
        int moveDown = GLFW_KEY_LEFT_CONTROL;
        int lookLeft = GLFW_KEY_LEFT;
        int lookRight = GLFW_KEY_RIGHT;
        int lookUp = GLFW_KEY_UP;
        int lookDown = GLFW_KEY_DOWN;
    };

    void moveInPlaneXZ(GLFWwindow* window, float dt, LveGameObject& gameObject);

    KeyMappings keys{};
    float moveSpeed{3.f};
    float lookSpeed{1.5f};

};



}// namespace lve


#endif /* KEYBOARD_MOVEMENT_CONTROLLER_H */
