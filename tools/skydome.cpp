#include <stdio.h>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "vars.hpp"

int g_circular_count = 0;
int g_radius_count = 0;

int getSkydomePosLength() {
   return g_circular_count * g_radius_count;
}

int getSkydomeElementLength() {
    return (g_circular_count+1) * 2 * (g_radius_count-1) + 2;
}

void updateSkydomeConf(int circular_count, int radius_count) {
    g_circular_count = circular_count;
    g_radius_count = radius_count;
}

glm::dvec3 calcMDSkyPosFromCoord(double lat, double lng) {
    return glm::dvec3(
        (double)localcons::earth_radius * std::cos(lat) * std::cos(lng) * 1.1 - vertex_offset.x,
        (double)localcons::earth_radius * std::sin(lat) * 1.1 - vertex_offset.y,
        (double)-localcons::earth_radius * std::cos(lat) * std::sin(lng) * 1.1 - vertex_offset.z
    );
}

void createSkydome(glm::vec3* vertex, int* vertex_offset, unsigned int* element_index, int* element_index_offset, glm::vec2 view_coord, int circular_count, float radius_range, int radius_count) {
    view_coord.x = view_coord.x/180.0f*localcons::pi;
    view_coord.y = view_coord.y/180.0f*localcons::pi;
    int vertex_offset_init = *vertex_offset;
    int pointer = vertex_offset_init;
    float circle_interval = 2*localcons::pi/circular_count;
    float radius_interval = radius_range/radius_count;
    for (int i=0; i<radius_count; i++) {
        for (int j=0; j<circular_count; j++) {
            vertex[pointer++] = calcMDSkyPosFromCoord(i * radius_interval * sin(j*circle_interval) + view_coord.x, i * radius_interval * cos(j*circle_interval) + view_coord.y);
        }
    }
    *vertex_offset = pointer;

    pointer = vertex_offset_init;
    int element_ptr = *element_index_offset;
    element_index[element_ptr++] = pointer;
    for (int i=0; i<radius_count-1; i++) {
        for (int j=0; j<circular_count; j++) {
            element_index[element_ptr++] = pointer + j + circular_count*i;
            element_index[element_ptr] = element_index[element_ptr-1] + circular_count;
            element_ptr++;
        }
        element_index[element_ptr++] = pointer + circular_count*i;
        element_index[element_ptr] = element_index[element_ptr-1] + circular_count;
        element_ptr++;
    }
    element_index[element_ptr] = element_index[element_ptr-1];
    *element_index_offset = element_ptr;
}
