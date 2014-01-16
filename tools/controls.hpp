#ifndef CONTROLS_HPP
#define CONTROLS_HPP

void sroll_callback(GLFWwindow* window, double xOffset, double yOffset);
void computeMatricesFromInputs();
void init_direction();
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();
void setPosCoord(float lat, float lng, float height);
extern glm::vec3 viewPos;

#endif
