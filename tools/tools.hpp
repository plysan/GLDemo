GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);
GLuint readTiffImage(char *name);
float* createLODDEM(int bl_coord_lat, int bl_coord_lng, int scale);
GLuint readTextureFromArray(glm::detail::uint32* array, GLuint dimension);
glm::vec3 calcFPosFromCoord(float lat, float lng);
glm::vec2 calcCoordFromPos(glm::vec3 pos);
double* midPos3D(double *a, double *b);
void doubleToGlmVec3(glm::vec3 *result, double *value);
