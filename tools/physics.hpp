class Object {
    public:
    float mass;
    glm::vec3 velocity;
    float force;
    glm::vec3 position;
    Object(glm::vec3);
    glm::vec3 nextPos();
};
extern glm::vec3* using_buffer_data;
extern glm::vec3 using_vertex_offset;
