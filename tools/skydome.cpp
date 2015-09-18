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

void createSkydome(glm::vec3* vertex, int* vertex_offset, unsigned int* element_index, int* element_index_offset, glm::vec2* vertex_uv, glm::vec2 view_coord, int circular_count, float radius_range, int radius_count) {
    view_coord.x = view_coord.x/180.0f*localcons::pi;
    view_coord.y = view_coord.y/180.0f*localcons::pi;
    int vertex_offset_init = *vertex_offset;
    int pointer = vertex_offset_init;
    float circle_interval = 2*localcons::pi/circular_count;
    float radius_interval = radius_range/radius_count;
    glm::vec2 zero_color_uv = glm::vec2(0.99f, 0.99f);
    for (int i=0; i<radius_count; i++) {
        for (int j=0; j<circular_count; j++) {
            vertex[pointer] = calcMDSkyPosFromCoord(i * radius_interval * sin(j*circle_interval) + view_coord.x, i * radius_interval * cos(j*circle_interval) + view_coord.y);
            vertex_uv[pointer++] = zero_color_uv;
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

void fillScatterTexture(glm::detail::uint32* scatter_texture_array_data, int dimension) {
    int dimension_exp_2 = dimension * dimension;
    float half_dimension_length = (float)(dimension-1)/2;
    glm::detail::uint32 horizontal_color = 16777215;
    glm::detail::uint32 vertical_color = 255;
    for (int i=0; i<dimension; i++) {
        float alpha_f = (-(float)i)/(dimension-1);
        for(int j=0; j<dimension; j++) {
            float color_factor = (half_dimension_length-j)/half_dimension_length;
            if(color_factor<0){color_factor = -color_factor;}
            color_factor = 1.0f - color_factor;
            glm::detail::uint32 color_r = (glm::detail::uint32)((int)(color_factor*(horizontal_color>>16&0x000000ff)) + (int)((1-color_factor)*(vertical_color>>16&0x000000ff)));
            glm::detail::uint32 color_g = (glm::detail::uint32)((int)(color_factor*(horizontal_color>>8&0x000000ff)) + (int)((1-color_factor)*(vertical_color>>8&0x000000ff)));
            glm::detail::uint32 color_b = (glm::detail::uint32)((int)(color_factor*(horizontal_color&0x000000ff)) + (int)((1-color_factor)*(vertical_color&0x000000ff)));
            glm::detail::uint32 alpha = (glm::detail::uint32)((alpha_f*j+dimension-1)/(dimension-1)*255.0f);
            for(int k=0; k<dimension; k++) {
                scatter_texture_array_data[i*dimension_exp_2 + j*dimension + k] = color_r<<24&0xff000000 | color_g<<16&0x00ff0000 | color_b<<8&0x0000ff00 | alpha&0x000000ff;
            }
        }
    }
}
