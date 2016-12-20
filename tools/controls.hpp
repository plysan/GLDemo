#ifndef CONTROLS_HPP
#define CONTROLS_HPP
#include "physics.hpp"

void sroll_callback(GLFWwindow* window, double xOffset, double yOffset);
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();
glm::vec3 setPosCoord(float lat, float lng, float height);
void computeMatricesFromInputs(Object* obj);

#endif
