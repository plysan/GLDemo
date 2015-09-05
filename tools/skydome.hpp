int getSkydomePosLength();
int getSkydomeElementLength();
void updateSkydomeConf(int circular_count, int radius_count);
void createSkydome(glm::vec3* vertex, int* vertex_offset, unsigned int* element_index, int* element_index_offset, glm::vec2 view_coord, int circular_count, float radius_range, int radius_count);
