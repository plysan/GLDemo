#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <sstream>
#include <thread>
#include "../main.hpp"
#include "../tools/physics.hpp"
#include "../tools/tools.hpp"

GLFWwindow* window;
short windowW = 1366, windowH = 768;
char window_title[10] = "holly";

long frame = 205;
long fps_frame_stamp = frame-1;
double fps_time_stamp = 0.0;

float horizontalAngle = 0.0f;
float verticalAngle = 1.57f;
float initialFoV = 45.0f;
float speed = 0.6f;
float mouseSpeed = 0.06f;
float srollSpeed = 1.0f;

glm::mat4 view_matrix;
glm::mat4 projection_matrix;
glm::vec3 up;
glm::vec3 tangent(1.0f, 1.0f, 1.0f);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void sroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
    initialFoV += yOffset * srollSpeed;
}

void updateWindowTitle() {
    while(true) {
        double time_now = glfwGetTime();
        float fps = (frame - fps_frame_stamp)/(time_now - fps_time_stamp);
        fps_frame_stamp = frame;
        fps_time_stamp = time_now;
        std::stringstream ss;
        ss << window_title << " [" << fps << " FPS]";
        glfwSetWindowTitle(window, ss.str().c_str());
        std::this_thread::sleep_for (std::chrono::seconds(1));
    }
}

void computeMatricesFromInputs(Object* obj){

    up = glm::normalize(toVec3(obj->using_vertex_offset) + obj->position);

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
    // Projection matrix : 45 degrees Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    projection_matrix = glm::perspective(initialFoV, (float)windowW / (float)windowH, 0.01f, 10000.0f);
    // Camera matrix
    direction *= 10000.0f;
    glm::vec3 posdir = obj->position+direction;
    view_matrix       = glm::lookAt(
                                obj->position,           // Camera is here
                                obj->position + direction, // and looks here : at the same viewPos, plus "direction"
                                up                  // Head is up (set to 0,-1,0 to look upside-down)
                           );

    // For the next frame, the "last time" will be "now"
    lastTime = currentTime;
}

void main_win_pre_loop() {
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow( windowW, windowH, window_title, NULL, NULL );
    if( !window )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, sroll_callback);

    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return;
    }

    //glfwSetWindowPos(window, 0, 0);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::thread first (updateWindowTitle);
    first.detach();
}

void main_win_loop() {
    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
    if (glfwWindowShouldClose(window)) {
        looping = false;
    }
}

void main_win_post_loop() {
    glfwTerminate();
}
