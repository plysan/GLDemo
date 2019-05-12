GLuint loadShaders(int argc, ...);
glm::vec3 calcFPosFromCoord(float lat, float lng);
glm::vec3 calcFPosFromCoord(float lat, float lng, float height);
glm::vec2 calcRadCoordFromPos(glm::vec3 pos);
glm::vec2 calcCoordFromPos(glm::vec3 pos);
glm::dvec2 calcDRadCoordFromDPos(glm::dvec3 pos);
void doubleToGlmVec3(glm::vec3 *result, double *value);
glm::vec3 toVec3(glm::dvec3 vec);
