#include <stdio.h>
#include<glm/glm.hpp>
#include<GL/glew.h>
#include"physics.hpp"
#include"constants.hpp"
#include"quardtree.hpp"
#include"tools.hpp"

float gravity = 0.1f;
float timeStep = 0.01f;

Object::Object(glm::vec3 pos) {
    position = pos;
    mass = 1.0f;
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 getResultNormalFromCoord(glm::vec2 coord, int* rIndex) {
    Node* targetNode = node;
    while (targetNode->bl != NULL) {
        if (targetNode->lat > coord.x) {
            if (targetNode->lng > coord.y) {
                targetNode = targetNode->bl;
            } else {
                targetNode = targetNode->br;
            }
        } else {
            if (targetNode->lng > coord.y) {
                targetNode = targetNode->tl;
            } else {
                targetNode = targetNode->tr;
            }
        }
    }
    float lat_offset = targetNode->lat + targetNode->nodeSize/2.0f - coord.x;
    float lng_offset = coord.y - targetNode->lng + targetNode->nodeSize/2.0f;
    float normal_index_row_offset = (lat_offset/targetNode->nodeSize * (float)(dinmension - 1));
    float normal_index_column_offset = (lng_offset/targetNode->nodeSize * (float)(dinmension - 1));
    int normal_index_offset = (int)normal_index_row_offset * dinmension + (int)normal_index_column_offset;
    float lat_deviation = normal_index_row_offset - (float)(int)normal_index_row_offset;
    float lng_deviation = normal_index_column_offset - (float)(int)normal_index_column_offset;
    int index = targetNode->start_index + normal_index_offset;
    *rIndex = index;
    if ((int)normal_index_row_offset%2 == 0) {
        if (lat_deviation + lng_deviation > 1.0f) {
            index++;
            *rIndex = index;
            return glm::normalize(glm::cross(result[index+dinmension-1] - result[index], result[index+dinmension] - result[index]));
        } else {
            return glm::normalize(glm::cross(result[index+dinmension] - result[index], result[index+1] - result[index]));
        }
    } else {
        if (1.0f - lat_deviation + lng_deviation > 1.0f) {
            return glm::normalize(glm::cross(result[index+dinmension+1] - result[index], result[index+1] - result[index]));
        } else {
            return glm::normalize(glm::cross(result[index+dinmension] - result[index], result[index+dinmension+1] - result[index]));
        }
    }
}

glm::vec3 Object::nextPos() {
    int result_index = 0;
    glm::vec3 resultNormal = getResultNormalFromCoord(calcCoordFromPos(position), &result_index);
    glm::vec3 facePoint_to_pos_vector = position - result[result_index];
    if (glm::dot(facePoint_to_pos_vector, resultNormal) < 0.0f) {
        position += -velocity * timeStep;
        velocity = glm::reflect(velocity, resultNormal) * 0.9f;
    } else {
        velocity += gravity * timeStep * glm::normalize(-position);
        position += velocity * timeStep;
    }
    return position;
}

