glm::vec3* createQuardTreePos();
glm::vec2* createQuardTreeUV();
glm::vec3* createNormal();
unsigned int* createQuardTreeElementIndex();
void createQuardTree(glm::vec2 bl_coord, glm::vec2 tr_coord, int* index, glm::vec3* result_ret, glm::vec2* result_uv_ret, glm::vec3* result_normal_ret, int* ele_index, unsigned int* result_index_ret, glm::detail::uint32* texture_array);

struct Node {
    Node* bl;
    Node* tl;
    Node* br;
    Node* tr;
    int start_index;
    float lat;
    float lng;
    float nodeSize;
};
extern Node* node;
