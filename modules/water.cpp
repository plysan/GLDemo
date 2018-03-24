#include <GL/glew.h>
#include <glm/glm.hpp>
#include <iostream>
#include <iomanip>
#include "../main.hpp"
#include "../tools/tools.hpp"
#include "../tools/vars.hpp"

GLuint water_buffer;
GLuint matrix_inverse_view_id;
GLuint matrix_inverse_perspective_id;
GLuint camera_pos_ms_id;
GLuint water_ac_uniform_id = -1;
GLuint cam_height_uniform_id = -1;
GLuint water_offset_uv_uniform_id = -1;
GLuint coord_length_uniform_id = -1;
GLuint water_origin_coord_uniform_id = -1;
int size_x = 200;
int size_y = 100;
int strip_length = ((2*size_x-1)*(size_y-1)+1)*2;

// after terrain pre loop
void water_pre_loop() {
    glm::vec3* water_grid = new glm::vec3[size_x*size_y*2];
    for(int i=0; i<size_y; i++) {
        for(int j=0; j<size_x; j++) {
            water_grid[(size_x*i+j)*2] = glm::vec3((j-size_x/2+0.5)/(size_x/2), (i-size_y/2+0.5)/(size_y/2), 0);
            water_grid[(size_x*i+j)*2+1] = glm::vec3((j-size_x+0.5)/size_x, (i-size_y+0.5)/size_y, 0);
        }
    }
    glm::vec3* water_grid_strip = new glm::vec3[strip_length];
    int index = 0, index_grid = 0, index_max = strip_length - 3;
    water_grid_strip[index++] = water_grid[index_grid*2];
    water_grid_strip[index++] = water_grid[index_grid*2+1];
    water_grid_strip[index++] = water_grid[(index_grid+size_x)*2];
    water_grid_strip[index++] = water_grid[(index_grid+size_x)*2+1];
    bool forward = true;
    index_grid++;
    while (index < index_max) {
        water_grid_strip[index++] = water_grid[index_grid*2];
        water_grid_strip[index++] = water_grid[index_grid*2+1];
        water_grid_strip[index++] = water_grid[(index_grid+size_x)*2];
        water_grid_strip[index++] = water_grid[(index_grid+size_x)*2+1];
        if ((index_grid%size_x == 0 || (index_grid+1)%size_x == 0) && index < index_max) {
            index_grid += size_x;
            water_grid_strip[index++] = water_grid[(index_grid+size_x)*2];
            water_grid_strip[index++] = water_grid[(index_grid+size_x)*2+1];
            forward = forward ? false : true;
        }
        index_grid = forward ? (index_grid+1) : (index_grid-1);
    }
    delete[] water_grid;
    glGenBuffers(1, &water_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, water_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*strip_length, water_grid_strip, GL_STREAM_DRAW);
    delete[] water_grid_strip;
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glBindBuffer(GL_ARRAY_BUFFER, water_buffer);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    matrix_inverse_view_id = glGetUniformLocation(programID, "inv_V");
    matrix_inverse_perspective_id = glGetUniformLocation(programID, "inv_P");
    camera_pos_ms_id = glGetUniformLocation(programID, "cam_ms");
    water_ac_uniform_id = glGetUniformLocation(programID, "water_ac");
    cam_height_uniform_id = glGetUniformLocation(programID, "cam_height");
    water_offset_uv_uniform_id = glGetUniformLocation(programID, "water_offset_uv");
    coord_length_uniform_id = glGetUniformLocation(programID, "coord_length");
    water_origin_coord_uniform_id = glGetUniformLocation(programID, "water_origin_coord");

    glm::detail::uint32* water_texture_array_data = new glm::detail::uint32[(int)pow(500, 2)];
    for (int i=0; i<250000; i++) {
        if (i%9==0) {
            water_texture_array_data[i] = 0x000000ff;
        } else {
            water_texture_array_data[i] = 0x00000000;
        }
    }
    glActiveTexture(GL_TEXTURE0 + water_texture_unit_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLuint water_texture_id;
    glGenTextures(1, &water_texture_id);
    glBindTexture(GL_TEXTURE_2D, water_texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, 500, 500,
        0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, water_texture_array_data);
    delete[] water_texture_array_data;
    GLuint water_texture_uniform_id  = glGetUniformLocation(programID, "water_texture_sampler");
    glUniform1i(water_texture_uniform_id, water_texture_unit_id);
}

void water_loop() {
    glUniform1i(render_target_uniform_id, 3);
    glm::mat4 matrix_inverse_perspective = glm::inverse(projection_matrix);
    glm::mat4 matrix_inverse_view = glm::inverse(view_matrix);
    glUniformMatrix4fv(matrix_inverse_view_id, 1, GL_FALSE, &matrix_inverse_view[0][0]);
    glUniformMatrix4fv(matrix_inverse_perspective_id, 1, GL_FALSE, &matrix_inverse_perspective[0][0]);
    glUniform3f(camera_pos_ms_id, viewObj->position.x, viewObj->position.y, viewObj->position.z);
    float water_ac = glm::pow(glm::length(glm::dvec3(viewObj->position) + glm::dvec3(viewObj->using_vertex_offset)), 2.0) - glm::pow(earth_radius, 2.0f);
    glUniform1f(water_ac_uniform_id, water_ac);
    glm::dvec3 cam_pos_earth = (glm::dvec3)viewObj->position + (glm::dvec3)viewObj->using_vertex_offset;
    float cam_height = glm::length(cam_pos_earth) - earth_radius;
    glUniform1f(cam_height_uniform_id, cam_height);
    glm::dvec2 cam_coord = calcDRadCoordFromDPos(cam_pos_earth);
    glm::vec2 water_origin_coord = glm::vec2(((int)(cam_coord.x/0.01f))*0.01f, ((int)(cam_coord.y/0.01f))*0.01f);
    glUniform2f(water_origin_coord_uniform_id, water_origin_coord.x, water_origin_coord.y);
    glm::vec2 offset_uv = glm::vec2((cam_coord.x - water_origin_coord.x) * water_uv_scale, (cam_coord.y - water_origin_coord.y) * water_uv_scale);
    glUniform2f(water_offset_uv_uniform_id, offset_uv.x, offset_uv.y);
    float lat_length = earth_radius / water_uv_scale;
    float lng_length = (double)earth_radius * cos(cam_coord.x) / water_uv_scale;
    glUniform2f(coord_length_uniform_id, lat_length, lng_length);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, strip_length/2);
}

void water_post_loop() {
}
