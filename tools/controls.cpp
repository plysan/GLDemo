#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "constants.hpp"

extern GLFWwindow* window;
extern short windowW, windowH;

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

glm::vec3 viewPos = glm::vec3( 0.0f, 0.0f, 0.0f ); 
float horizontalAngle = 0.0f;
float verticalAngle = 1.57f;
float initialFoV = 45.0f;
float speed = 100.0f;
float mouseSpeed = 0.06f;

void sroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
    initialFoV += yOffset;
}

void setPosCoord(float lat, float lng){
    lat = lat/180*localcons::pi;
    lng = lng/180*localcons::pi;
    viewPos.x = localcons::earth_radius * cos(lat) * cos(lng);
    viewPos.y = localcons::earth_radius * sin(lat);
    viewPos.z = localcons::earth_radius * cos(lat) * sin(lng);
    up = glm::normalize(viewPos);
    printf("viewPos: (%f, %f, %f)\n", viewPos.x, viewPos.y, viewPos.z);
}

void computeMatricesFromInputs(){

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
		viewPos += direction * deltaTime * speed;
	}
	// Move backward
	if (glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS){
		viewPos -= direction * deltaTime * speed;
	}
	// Strafe right
	if (glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS){
		viewPos += right * deltaTime * speed;
	}
	// Strafe left
	if (glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS){
		viewPos -= right * deltaTime * speed;
	}
    if (glfwGetKey( window, GLFW_KEY_SPACE ) == GLFW_PRESS){
		viewPos += up * deltaTime * speed;
	}
    if (glfwGetKey( window, GLFW_KEY_LEFT_SHIFT ) == GLFW_PRESS){
		viewPos -= up * deltaTime * speed;
	}
	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	ProjectionMatrix = glm::perspective(initialFoV, (float)windowW / (float)windowH, 0.01f, 10000.0f);
	// Camera matrix
    direction *= 10000.0f;
    glm::vec3 posdir = viewPos+direction;
	ViewMatrix       = glm::lookAt(
								viewPos,           // Camera is here
								viewPos + direction, // and looks here : at the same viewPos, plus "direction"
								up                  // Head is up (set to 0,-1,0 to look upside-down)
						   );

	// For the next frame, the "last time" will be "now"
	lastTime = currentTime;
}
