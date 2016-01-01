extern glm::vec3* result;
extern int dinmension;
extern int terrain_texture_size;
extern int maxNodes;
extern int nodeIndex;
extern int texture_unit_index;
extern int vertexBufferSize;
extern int ele_index_size;
extern glm::vec3 vertex_offset;
extern glm::vec3 vertex_offset_diff;

int getQuardTreePosLength();
int getQuardTreeUVLength();
int getQuardTreeNormalLength();
int getQuardTreeElementIndexLength();

struct Node {
    Node* bl;
    Node* tl;
    Node* br;
    Node* tr;
    int start_index;
    float lat_mid;
    float lng_mid;
    float lat_bl;
    float lng_bl;
    float node_size_lat;
    float node_size_lng;
};
extern Node* node;
void createQuardTree(glm::vec2 bl_coord, glm::vec2 tr_coord, int* index, glm::vec3* result_ret, glm::vec2* result_uv_ret, glm::vec3* result_normal_ret, int* ele_index, unsigned int* result_index_ret, glm::detail::uint32* texture_array, Node** new_node);
void cleanupNode(Node** node);
