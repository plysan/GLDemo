#ifndef QUARDTREETERRAIN_HPP
#define QUARDTREETERRAIN_HPP
#include <tiffio.h>

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

class QTProfile {
  public:
    // offset when start generating quardtree
    glm::vec3 vertex_offset;

    Node* node = NULL;
    glm::vec3* result;
    glm::vec2* result_uv;
    unsigned int* result_index;
    glm::vec3* result_normal;

    float elevation_divisor;

    // dinmension must be 2^n+1 where n is int
    int dinmension;
    int ele_index_node_size;
    int maxNodes;
    int vertexBufferSize;
    int ele_index_size;
    int nodeIndex = 0;
    int vertex_index_offset = 0;
    int element_index_offset = 0;
    float minNodeSize;
    float maxNodeSize;
    float one_degree_lat_length;

    uint32* texture;
    int texture_unit_size;
    int texture_unit_size_dem;
    int texture_unit_dinmension;
    int terrain_texture_size;
    int terrain_texture_units;
    int texture_unit_index = 0;
    int lod_max;

    int quardtree_pos_length;
    int quardtree_uv_length;
    int quardtree_element_index_length;
    int quardtree_normal_length;
    int terrain_texture_length;

    QTProfile(
        int dinmension,
        int maxNodes,
        float minNodeSize,
        float maxNodeSize,
        int texture_unit_size,
        int texture_unit_size_dem,
        int texture_unit_dinmension,
        int lod_max);
    void updateValues();
    void createQuardTree(
        glm::vec2 bl_coord,
        glm::vec2 tr_coord,
        int* index,
        glm::vec3* result_ret,
        glm::vec2* result_uv_ret,
        glm::vec3* result_normal_ret,
        int* ele_index,
        unsigned int* result_index_ret,
        glm::detail::uint32* texture_array,
        Node** new_node,
        glm::vec3* viewing_pos);
    void cleanupNode(Node** node);
    glm::vec3 getResultNormalFromCoord(glm::vec2 coord, glm::vec3* using_buffer_data, int* rIndex);

  private:
    glm::dvec3 calcMDTerrainPosFromCoord(double lat, double lng);
    void interpolatePos2D(
        glm::dvec2 bl_coord,
        glm::dvec2 tr_coord,
        glm::vec2 bl_uv,
        glm::vec2 tr_uv,
        int mid_pos_index,
        int unit_size);
    void interpolatePos1D(
        glm::dvec2 frst_coord,
        glm::dvec2 lst_coord,
        glm::vec2 frst_uv,
        glm::vec2 lst_uv,
        int mid_pos_index,
        int interval,
        int unit_size);
    void elevationOffset(glm::vec3 *result, double elevation_factor);
    void addNodeToResult(
        glm::vec2 bl_coord,
        glm::vec2 tr_coord,
        glm::vec2 bl_uv,
        glm::vec2 tr_uv,
        Node** node);
    float calResultNormalwithRoughness(
        int ele_index,
        int offset_a,
        int offset_b,
        bool add_roughness);
    float genNodeElementNormalwithRoughness(int idx);
    bool getImageFromCoords(
        TIFF** tif,
        glm::vec2* image_bl_coord,
        float* span_image_coord,
        glm::vec2 bl_coord,
        glm::vec2 tr_coord);
    bool readImageToTexture(
        glm::vec2 bl_coord,
        glm::vec2 tr_coord,
        int scale_x,
        int scale_y,
        int base_index_unit);
    bool readGlobalImageToTexture(glm::vec2 bl_coord, glm::vec2 tr_coord);
    glm::vec2 getNewUv();
    glm::vec2 new_texture_unit(glm::vec2 bl_coord, glm::vec2 tr_coord, bool detailed);
    void selectNode(
        glm::vec2 bl_coord,
        glm::vec2 tr_coord,
        glm::vec2 bl_uv,
        glm::vec2 tr_uv,
        int level,
        Node** node,
        glm::vec3* viewing_pos);
};

#endif

