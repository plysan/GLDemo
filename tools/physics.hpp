class Object {
    public:
    float mass;
    glm::vec3 velocity;
    float force;
    glm::vec3 position;
    Object(glm::vec3);
    glm::vec3 nextPos();
};

