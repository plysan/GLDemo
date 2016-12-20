#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "vars.hpp"
#include "tools.hpp"
#include "physics.hpp"
#include "quardtreeTerrain.hpp"

extern GLFWwindow* window;
extern short windowW, windowH;
extern QTProfile qt_terrain;

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix(){
    return ViewMatrix;
}
glm::mat4 getProjectionMatrix(){
    return ProjectionMatrix;
}
glm::vec3 up;
glm::vec3 tangent(1.0f, 1.0f, 1.0f);

float horizontalAngle = 0.0f;
float verticalAngle = 1.57f;
float initialFoV = 45.0f;
float speed = 0.6f;
float mouseSpeed = 0.06f;
float srollSpeed = 1.0f;

void sroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
    initialFoV += yOffset * srollSpeed;
}

glm::vec3 setPosCoord(float lat, float lng, float height){
    return calcFPosFromCoord(lat, lng) * (height+earth_radius)/earth_radius;
}

void computeMatricesFromInputs(Object* obj){

    up = glm::normalize(obj->using_vertex_offset + obj->position);

    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    float deltaTime = (float)(currentTime - lastTime);

    // Get mouse viewPos
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Reset mouse viewPos for next frame
    glfwSetCursorPos(window, windowW/2, windowH/2);

    // Compute new orientation
    horizontalAngle += mouseSpeed * float( windowW/2 - xpos );
    verticalAngle += mouseSpeed * float( windowH/2 - ypos );

    glm::vec3 direction = glm::cross(up, tangent);
    direction = glm::rotate(direction, horizontalAngle, up);
    glm::vec3 horotateaxis = glm::cross(direction, up);
    direction = glm::rotate(direction, verticalAngle, horotateaxis);
    glm::vec3 right = glm::cross(direction, up);

    // Move forward
    if (glfwGetKey( window, GLFW_KEY_W ) == GLFW_PRESS){
        obj->velocity += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS){
        obj->velocity -= direction * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS){
        obj->velocity += right * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS){
        obj->velocity -= right * deltaTime * speed;
    }
    if (glfwGetKey( window, GLFW_KEY_SPACE ) == GLFW_PRESS){
        obj->velocity += up * deltaTime * speed;
    }
    if (glfwGetKey( window, GLFW_KEY_LEFT_SHIFT ) == GLFW_PRESS){
        obj->velocity -= up * deltaTime * speed;
    }
    obj->position = obj->nextPosOf(&qt_terrain);
    // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    ProjectionMatrix = glm::perspective(initialFoV, (float)windowW / (float)windowH, 0.01f, 10000.0f);
    // Camera matrix
    direction *= 10000.0f;
    glm::vec3 posdir = obj->position+direction;
    ViewMatrix       = glm::lookAt(
                                obj->position,           // Camera is here
                                obj->position + direction, // and looks here : at the same viewPos, plus "direction"
                                up                  // Head is up (set to 0,-1,0 to look upside-down)
                           );

    // For the next frame, the "last time" will be "now"
    lastTime = currentTime;
}
