#include <stdio.h>
#include<glm/glm.hpp>
#include<GL/glew.h>
#include"physics.hpp"
#include"constants.hpp"
#include"quardtree.hpp"
#include"tools.hpp"

float gravity = 0.1f;
float timeStep = 0.01f;

glm::vec3* using_buffer_data;
// indicates the offset of the rendering buffer
glm::vec3 using_vertex_offset;

Object::Object(glm::vec3 pos) {
    position = pos;
    mass = 1.0f;
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 getResultNormalFromCoord(glm::vec2 coord, int* rIndex) {
    Node* target_node = node;
    Node* target_node_suspect = node;
    while (target_node_suspect != NULL) {
        target_node = target_node_suspect;
        if (target_node->lat > coord.x) {
            if (target_node->lng > coord.y) {
                target_node_suspect = target_node->bl;
            } else {
                target_node_suspect = target_node->br;
            }
        } else {
            if (target_node->lng > coord.y) {
                target_node_suspect = target_node->tl;
            } else {
                target_node_suspect = target_node->tr;
            }
        }
    }
    float lat_offset = target_node->lat + target_node->nodeSize/2.0f - coord.x;
    float lng_offset = coord.y - target_node->lng + target_node->nodeSize/2.0f;
    float normal_index_row_offset = (lat_offset/target_node->nodeSize * (float)(dinmension - 1));
    float normal_index_column_offset = (lng_offset/target_node->nodeSize * (float)(dinmension - 1));
    int normal_index_offset = (int)normal_index_row_offset * dinmension + (int)normal_index_column_offset;
    float lat_deviation = normal_index_row_offset - (float)(int)normal_index_row_offset;
    float lng_deviation = normal_index_column_offset - (float)(int)normal_index_column_offset;
    int index = target_node->start_index + normal_index_offset;
    *rIndex = index;
    if ((int)normal_index_row_offset%2 == 0) {
        if (lat_deviation + lng_deviation > 1.0f) {
            index++;
            *rIndex = index;
            return glm::normalize(glm::cross(using_buffer_data[index+dinmension-1] - using_buffer_data[index], using_buffer_data[index+dinmension] - using_buffer_data[index]));
        } else {
            return glm::normalize(glm::cross(using_buffer_data[index+dinmension] - using_buffer_data[index], using_buffer_data[index+1] - using_buffer_data[index]));
        }
    } else {
        if (1.0f - lat_deviation + lng_deviation > 1.0f) {
            return glm::normalize(glm::cross(using_buffer_data[index+dinmension+1] - using_buffer_data[index], using_buffer_data[index+1] - using_buffer_data[index]));
        } else {
            return glm::normalize(glm::cross(using_buffer_data[index+dinmension] - using_buffer_data[index], using_buffer_data[index+dinmension+1] - using_buffer_data[index]));
        }
    }
}

glm::vec3 Object::nextPos() {
    if (node == NULL) {
        return glm::vec3(0.0f);
    }
    int using_buffer_data_index = 0;
    glm::vec3 using_buffer_dataNormal = getResultNormalFromCoord(calcCoordFromPos(position + using_vertex_offset), &using_buffer_data_index);
    glm::vec3 facePoint_to_pos_vector = position - using_buffer_data[using_buffer_data_index];
    if (glm::dot(facePoint_to_pos_vector, using_buffer_dataNormal) < 0.0f) {
        position += -velocity * timeStep;
        velocity = glm::reflect(velocity, using_buffer_dataNormal) * 0.9f;
    } else {
        velocity += gravity * timeStep * glm::normalize(-position - vertex_offset);
        position += velocity * timeStep;
    }
    return position;
}

