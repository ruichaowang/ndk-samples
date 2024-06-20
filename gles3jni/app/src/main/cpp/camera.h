#ifndef CAMERA_H
#define CAMERA_H

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW         = -0.0f;
const float PITCH       =  89.0f;
const float SPEED       =  5.0f;
const float SENSITIVITY =  0.05f;
const float ZOOM        =  45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // 四元数代替Yaw和Pitch
    glm::quat Orientation;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Orientation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.0f));
        updateCameraVectors();
    }
    // // // constructor with scalar values

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        // make sure the user stays at the ground level
        // Position.y = 0.0f; // <-- this one-liner keeps the user at the ground level (xz plane)
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

         // 四元数表示的旋转，围绕x轴旋转pitch角度，围绕y轴旋转yaw角度
        glm::quat qPitch = glm::angleAxis(glm::radians(yoffset), glm::vec3(1, 0, 0));
        glm::quat qYaw = glm::angleAxis(glm::radians(xoffset), glm::vec3(0, 0, 1)); // 注意这里的旋转轴是z轴

        // Right now applying pitch first then yaw.
        Orientation = qYaw * Orientation * qPitch;
        Orientation = glm::normalize(Orientation); // 四元数需要归一化
        
        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f; 
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // 将Orientation四元数转换为3x3旋转矩阵
        glm::mat3 rotMatrix = glm::mat3_cast(Orientation);
        // 现在可以使用旋转矩阵来旋转向量
        Front = rotMatrix * glm::vec3(0.0f, 0.0f, -1.0f);
        Right = rotMatrix * glm::vec3(1.0f, 0.0f, 0.0f);
        Up    = glm::cross(Right, Front);
    }
};
#endif
