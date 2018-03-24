#include <glm/glm.hpp>
#include <GL/glew.h>
#include "physics.hpp"
#include "vars.hpp"
#include "quardtreeTerrain.hpp"
#include "tools.hpp"

Object::Object() {
    position = glm::vec3();
    mass = 1.0f;
    velocity = glm::vec3();
}

glm::vec3 Object::nextPosOf(QTProfile* terrain) {
    if (terrain->node == NULL) {
        return glm::vec3(0.0f);
    }
    int using_buffer_data_index = 0;
    glm::vec3 using_buffer_dataNormal = terrain->getResultNormalFromCoord(calcCoordFromPos(position + toVec3(using_vertex_offset)), using_vertex,  &using_buffer_data_index);
    glm::vec3 facePoint_to_pos_vector = position - using_vertex[using_buffer_data_index];
    if (glm::dot(facePoint_to_pos_vector, using_buffer_dataNormal) < 0.0f) {
        position += -velocity * timeStep;
        velocity = glm::reflect(velocity, using_buffer_dataNormal) * 0.9f;
    } else {
        velocity += gravity * timeStep * glm::normalize(-position - toVec3(using_vertex_offset));
        position += velocity * timeStep;
    }
    return position;
}

