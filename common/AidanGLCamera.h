

#ifndef AGLCAMERA_H
#define AGLCAMERA_H

//General includes
#include <math.h>

//Opengl includes
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

// Camera class, basically just generates a viewmatrix
class Camera {
    public:
        Camera()
        {}
        void init(GLFWwindow *cameraWindow, glm::vec3 pos, glm::mat4 perspective, float horizontalAngle, float verticalAngle, float speed, float sens, bool inputType)
        {
            // Initialize basic data
            window = cameraWindow;
            glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &windowWidth, &windowHeight, &windowSizeX, &windowSizeY);
            position = pos;
            angleX = horizontalAngle;
            angleY = verticalAngle;
            perspectiveMatrix = perspective;
            cameraSpeed = speed;
            mouseSensitivity = sens;
            allowInput = inputType;

            lastTime = glfwGetTime();
            update();
        }
        void update()
        {
            // Figure out how long it's been since the last frame
            currentTime = glfwGetTime();
            deltaTime = currentTime - lastTime;

            // Sometimes need to disable user input
            if(allowInput)
            {
                // Get mouse position
                double mouseX, mouseY;
                glfwGetCursorPos(window, &mouseX, &mouseY);
                // Need to reset cursor to a known location
                glfwSetCursorPos(window, windowSizeX/2, windowSizeY/2);

                // Calculate viewing angles based on mouse movement
                angleX += mouseSensitivity * (float)deltaTime * (windowSizeX/2 - (float)mouseX);
                angleY += mouseSensitivity * (float)deltaTime * (windowSizeY/2 - (float)mouseY);

                updateCameraDirection();        // update direction vector
                updateCameraRight();            // update vector facing to the right of the camera, to calculate the up vector
                updateCameraUp();               // update the up vector
                updateViewMatrix();             // calculate viewmatrix

                // Check keyboard input for camera movement
                int upKeyState = glfwGetKey(window, GLFW_KEY_W);
                int leftKeyState = glfwGetKey(window, GLFW_KEY_A);
                int downKeyState = glfwGetKey(window, GLFW_KEY_S);
                int rightKeyState = glfwGetKey(window, GLFW_KEY_D);
                int spaceKeyState = glfwGetKey(window, GLFW_KEY_SPACE);
                int shiftKeyState = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
                // Move forward
                if(upKeyState == GLFW_PRESS)
                {
                    position += direction * (float)deltaTime * cameraSpeed;
                }
                // Move backward
                if(downKeyState == GLFW_PRESS)
                {
                    position -= direction * (float)deltaTime * cameraSpeed;
                }
                // Move right
                if(rightKeyState == GLFW_PRESS)
                {
                    position += right * (float)deltaTime * cameraSpeed;
                }
                // Move left
                if(leftKeyState == GLFW_PRESS)
                {
                    position -= right * (float)deltaTime * cameraSpeed;
                }
                // Move up
                if(spaceKeyState == GLFW_PRESS)
                {
                    position += up * (float)deltaTime * cameraSpeed;
                }
                // Move down
                if(shiftKeyState == GLFW_PRESS)
                {
                    position -= up * (float)deltaTime * cameraSpeed;
                }
            }
            // timing data
            lastTime = currentTime;
        }
        glm::mat4 getProjectionMatrix()
        {
            return perspectiveMatrix;
        }
        void setProjectionMatrix(glm::mat4 perspective)
        {
            perspectiveMatrix = perspective;
        }
        glm::vec3 getPosition()
        {
            return position;
        }
        void setPosition(glm::vec3 pos)
        {
            position = pos;
        }
        glm::mat4 getViewMatrix()
        {
            return viewMatrix;
        }
        void disableUserInput()
        {
            allowInput = false;
        }
        void enableUserInput()
        {
            allowInput = true;
        }
    private:
        GLFWwindow *window;                                         // Camera needs to know what window it's rendering in
        glm::mat4 viewMatrix, perspectiveMatrix;
        glm::vec3 position, direction, right, up;
        int windowWidth, windowHeight, windowSizeX, windowSizeY;
        float cameraSpeed, mouseSensitivity, angleX, angleY;
        double lastTime, currentTime, deltaTime;
        bool allowInput;
        
        glm::vec3 getCameraDirection()
        {
            return direction;
        }
        void updateCameraDirection()
        {
            // necessary math
            direction = glm::vec3(
                cos(angleY) * sin(angleX),
                sin(angleY),
                cos(angleY) * cos(angleX)
            );
        }
        void setCameraDirection()
        {
            //TODO
        }
        void updateCameraRight()
        {
            // also necessary math
            right = glm::vec3(
                sin(angleX - 3.141592654/2.0),
                0,
                cos(angleX - 3.141592654/2.0)
            );
        }
        glm::vec3 getCameraRight()
        {
            return right;
        }
        void updateCameraUp()
        {
            up = glm::cross(right, direction);
        }
        void updateViewMatrix()
        {
            // use glm's handy lookAt function to create viewMatrix
            viewMatrix = glm::lookAt(
                position,
                position+direction,
                up
            );
        }
};


#endif