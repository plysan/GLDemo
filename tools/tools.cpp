#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <tiffio.h>
#include <glm/glm.hpp>
#include "vars.hpp"
#include "quardtreeTerrain.hpp"

using namespace std;

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

glm::vec2 calcCoordFromPos(glm::vec3 pos) {
    float radius_xz = sqrt(pos.x*pos.x + pos.z*pos.z);
    float lat = atan(pos.y/radius_xz)/pi*180.0f;
    float lng = -atan(pos.z/pos.x);
    if (pos.x < 0.0f) {
        if (lng < 0.0f) {
            lng += pi;
        } else {
            lng -= pi;
        }
    }
    lng = lng/pi*180.0f;
    return glm::vec2(lat, lng);
}

// shader loading

string* readShaderCode(const char* file_path) {
    string* shader_code = new string();
    std::ifstream shader_stream(file_path, std::ios::in);
    if(shader_stream.is_open()){
        std::string line = "";
        while(getline(shader_stream, line))
            *shader_code += "\n" + line;
        shader_stream.close();
        return shader_code;
    }else{
        printf("Error opening %s.\n", file_path);
        return NULL;
    }
}

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path, const char* global_vars_path){

    // Create the shaders
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    string** shader_strings = new string*[2];
    const char** source_pointes = new const char*[3];
    source_pointes[0] = "#version 330 core\n";

    shader_strings[0] = readShaderCode(global_vars_path);
    shader_strings[1] = readShaderCode(vertex_file_path);
    source_pointes[1] = shader_strings[0]->c_str();
    source_pointes[2] = shader_strings[1]->c_str();
    glShaderSource(vertex_shader_id, 3, source_pointes, NULL);
    glCompileShader(vertex_shader_id);

    shader_strings[0] = readShaderCode(global_vars_path);
    shader_strings[1] = readShaderCode(fragment_file_path);
    source_pointes[1] = shader_strings[0]->c_str();
    source_pointes[2] = shader_strings[1]->c_str();
    glShaderSource(fragment_shader_id, 3, source_pointes, NULL);
    glCompileShader(fragment_shader_id);

    delete[] shader_strings;
    delete[] source_pointes;

    // Link the program
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

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

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    return program_id;
}
