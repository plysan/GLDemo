GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);
GLuint readTiffImage(char *name);
float* createLODDEM(int bl_coord_lat, int bl_coord_lng, int scale);
GLuint readTextureFromArray(float* array, GLuint dimension);
glm::vec3 calcPosFromCoord(float lat, float lng);
