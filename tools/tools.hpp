GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);
glm::vec3 calcFPosFromCoord(float lat, float lng);
glm::dvec3 calcMDPosFromCoord(float lat, float lng);
glm::vec2 calcCoordFromPos(glm::vec3 pos);
void doubleToGlmVec3(glm::vec3 *result, double *value);
