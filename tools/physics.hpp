#ifndef PHYSICS_HPP
#define PHYSICS_HPP
#include "quardtreeTerrain.hpp"

class Object {
  public:
    float mass;
    glm::vec3 velocity;
    float force;
    glm::vec3 position;
    glm::vec3* using_vertex;
    // indicates the offset of the rendering buffer
    glm::dvec3 using_vertex_offset;
    float timeStep = 0.01f;

    Object();
    glm::vec3 nextPosOf(QTProfile* terrain);
};

#endif
