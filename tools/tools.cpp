#include <stdio.h>
#include <stdlib.h>
#include <cstdarg>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <tiffio.h>
#include <stdexcept>
#include <glm/glm.hpp>
#include "vars.hpp"
#include "quardtreeTerrain.hpp"

#include <GL/glew.h>

glm::vec3 calcFPosFromCoord(float lat, float lng) {
    lat = lat/180*pi;
    lng = lng/180*pi;
    return glm::vec3(
            earth_radius * cos(lat) * cos(lng),
            earth_radius * sin(lat),
            -earth_radius * cos(lat) * sin(lng)
            );
}

glm::vec3 calcFPosFromCoord(float lat, float lng, float height) {
    return calcFPosFromCoord(lat, lng) * (height+earth_radius)/earth_radius;
}

glm::vec2 calcRadCoordFromPos(glm::vec3 pos) {
    float radius_xz = sqrt(pos.x*pos.x + pos.z*pos.z);
    float lat = atan(pos.y/radius_xz);
    float lng = -atan(pos.z/pos.x);
    if (pos.x < 0.0f) {
        if (lng < 0.0f) {
            lng += pi;
        } else {
            lng -= pi;
        }
    }
    return glm::vec2(lat, lng);
}

glm::vec2 calcCoordFromPos(glm::vec3 pos) {
    return calcRadCoordFromPos(pos)/pi*180.0f;
}

glm::dvec2 calcDRadCoordFromDPos(glm::dvec3 pos) {
    double radius_xz = sqrt(pos.x*pos.x + pos.z*pos.z);
    double lat = atan(pos.y/radius_xz);
    double lng = -atan(pos.z/pos.x);
    if (pos.x < 0.0) {
        if (lng < 0.0) {
            lng += pi;
        } else {
            lng -= pi;
        }
    }
    return glm::dvec2(lat, lng);
}

glm::vec3 toVec3(glm::dvec3 vec) {
    return glm::vec3((float)vec.x, (float)vec.y, (float)vec.z);
}

// shader loading

void readShaderCode(const char* file_path, char** content) {
    std::string* shader_code = new std::string();
    std::ifstream shader_stream(file_path, std::ios::in);
    if(shader_stream.is_open()){
        std::string line = "";
        while(getline(shader_stream, line))
            *shader_code += "\n" + line;
        shader_stream.close();
        const std::string::size_type size = shader_code->size();
        *content = new char[size + 1];
        memcpy(*content, shader_code->c_str(), size+1);
    }else{
        printf("Error opening %s.\n", file_path);
    }
    delete shader_code;
}

GLuint loadShaders(int argc, ...) {
    va_list args;
    va_start(args, argc);
    GLuint program_id = glCreateProgram();
    char** sources_list = new char*[3];
    sources_list[0] = (char*)"#version 330 core\n";
    readShaderCode("tools/vars.hpp", &sources_list[1]);
    for (int i=0; i<argc; i++) {
        const char* shader_path = va_arg(args, const char*);
        GLenum shader_type;
        int shader_path_len = strlen(shader_path);
        if (strcmp(shader_path+shader_path_len-5, ".vert") == 0)
            shader_type = GL_VERTEX_SHADER;
        else if (strcmp(shader_path+shader_path_len-5, ".frag") == 0)
            shader_type = GL_FRAGMENT_SHADER;
        else
            throw std::runtime_error(std::string("Error reading shader file: ") + shader_path);
        GLuint shader_id = glCreateShader(shader_type);
        readShaderCode(shader_path, &sources_list[2]);
        glShaderSource(shader_id, 3, sources_list, NULL);
        glCompileShader(shader_id);
        glAttachShader(program_id, shader_id);
        glDeleteShader(shader_id);
        delete[] sources_list[2];
    }
    glLinkProgram(program_id);
    delete[] sources_list[1];
    delete[] sources_list;

    // Check the program
    GLint result = GL_FALSE;
    int info_log_length;
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if ( info_log_length > 0 ){
        std::vector<char> program_error_message(info_log_length+1);
        glGetProgramInfoLog(program_id, info_log_length, NULL, &program_error_message[0]);
        printf("Error compiling shaders:\n%s\n", &program_error_message[0]);
    }

    return program_id;
}
