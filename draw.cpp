#include <stdio.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <thread>
#include <chrono>
#include "tools/tools.hpp"
#include "tools/controls.hpp"
#include "tools/quardtreeTerrain.hpp"
#include "tools/physics.hpp"
#include "tools/skydome.hpp"
#include "tools/spaceCube.hpp"
#include "tools/vars.hpp"

extern glm::vec3 viewPos;
// used to store vertex offset of the updating buffer and update to physics in the next buffer update
glm::vec3 vertex_offset_snap;

Node* new_node = NULL;
Node* node_to_del = NULL;

GLFWwindow* window;
short windowW = 1366, windowH = 768;
char window_title[10] = "holly";

bool updating = true;
bool unmapping = false;
bool terminating = false;

glm::vec3** g_vertex_buffer_data;
glm::vec3* g_mapped_vertex_buffer_data;
glm::vec2* g_mapped_vertex_uv_data;
glm::vec3* g_mapped_vertex_normal_data;
unsigned int* g_mapped_vertex_element_data;
glm::detail::uint32* g_mapped_terrain_texture_array_data;
int elemant_index_terrain_length_update = 0;
int elemant_index_terrain_length_rendering = 0;
int elemant_index_sky_length_update = 0;
int elemant_index_sky_length_rendering = 0;
int vertex_static_data_length = 0;
int element_static_data_length = 0;

long frame_counter = 205;
long fps_frame_stamp = frame_counter-1;
double fps_time_stamp = 0.0;
int renderingBufferIndex = 0;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void updateWindowTitle() {
    double time_now = glfwGetTime();
    float fps = (frame_counter - fps_frame_stamp)/(time_now - fps_time_stamp);
    fps_frame_stamp = frame_counter;
    fps_time_stamp = time_now;
    std::stringstream ss;
    ss << window_title << " [" << fps << " FPS]";
    glfwSetWindowTitle(window, ss.str().c_str());
}

void updateData(bool loop)
{
    do {
        if (terminating) {
            terminating = false;
            return;
        }
        if (!updating) {
            continue;
        }
        updateWindowTitle();
        int vertex_pointer = vertex_static_data_length;
        int element_pointer = element_static_data_length;
        int element_pointer_old = element_pointer;
        clock_t before = clock();
        vertex_offset_diff = viewPos;
        vertex_offset += vertex_offset_diff;
        vertex_offset_snap = vertex_offset;
        new_node = new Node;
        cleanupNode(&node_to_del);
        createQuardTree(
            glm::vec2(-20.0f, -180.0f),
            glm::vec2(60.0f, -100.0f),
            &vertex_pointer,
            g_vertex_buffer_data[renderingBufferIndex],
            g_mapped_vertex_uv_data,
            g_mapped_vertex_normal_data,
            &element_pointer,
            g_mapped_vertex_element_data,
            g_mapped_terrain_texture_array_data,
            &new_node
            );
        elemant_index_terrain_length_update = element_pointer - element_pointer_old;
        element_pointer_old = element_pointer;
        glm::vec2 view_coord = calcCoordFromPos(vertex_offset);
        createSkydome(
            g_vertex_buffer_data[renderingBufferIndex], &vertex_pointer,
            g_mapped_vertex_element_data, &element_pointer,
            g_mapped_vertex_uv_data,
            &view_coord, 64, 1.5f, 512);
        elemant_index_sky_length_update = element_pointer - element_pointer_old;
        printf("execution time: %fs ", (double)(clock() - before)/CLOCKS_PER_SEC);
        printf("points: %d, indices: %d, nodes:%d, texture_nodes:%d\n", vertex_pointer, element_pointer, nodeIndex, texture_unit_index);

        std::copy(&g_vertex_buffer_data[renderingBufferIndex][vertex_static_data_length], &g_vertex_buffer_data[renderingBufferIndex][vertex_pointer], &g_mapped_vertex_buffer_data[vertex_static_data_length]);

        unmapping = true;
        updating = false;
        std::this_thread::sleep_for (std::chrono::seconds(1));
    } while (loop);
}

GLuint pixelBuffer;
GLuint* vertexbuffer = new GLuint[2];;
GLuint* uvbuffer = new GLuint[2];;
GLuint* normalbuffer = new GLuint[2];;
GLuint* elementBuffer = new GLuint[2];;

void unmapBuffers() {
    //unmapping the updated buffers
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[renderingBufferIndex]);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[renderingBufferIndex]);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[renderingBufferIndex]);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, elementBuffer[renderingBufferIndex]);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    unmapping = false;
}

int main( void )
{
    int scatter_texture_size = scatter_texture_3d_size*scatter_texture_4thd_in_3d_size;
    glm::detail::uint32* g_scatter_texture_array_data = new glm::detail::uint32[(int)pow(scatter_texture_size, 3)];
    fillScatterTexture(g_scatter_texture_array_data, scatter_texture_3d_size, scatter_texture_4thd_in_3d_size);

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow( windowW, windowH, window_title, NULL, NULL );
    if( !window )
    {
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, sroll_callback);

    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    glfwSetCursorPos(window, windowW/2, windowH/2);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("tools/VertexShader", "tools/FragmentShader", "tools/vars.hpp");
    GLuint matrixMVPID = glGetUniformLocation( programID, "MVP" );
    GLuint matrixMID = glGetUniformLocation( programID, "M" );
    GLuint matrixVID = glGetUniformLocation( programID, "V" );
    GLuint matrix_notrans_VP_ID = glGetUniformLocation( programID, "notrans_VP" );
    GLuint sun_worldspace_uniform_id = glGetUniformLocation( programID, "sun_ws" );
    GLuint vertex_offset_uniform_id = glGetUniformLocation(programID, "vertex_offset_ws");
    GLuint scatter_height_uniform_id = glGetUniformLocation(programID, "scatter_height");
    GLuint render_target_uniform_id = glGetUniformLocation(programID, "render_target");
    glUseProgram(programID);

    setPosCoord(20.0f, -156.0f, 0.7f);
    updateSkydomeConf(64, 512);
    int vertex_buffer_length = getQuardTreePosLength() + getSkydomePosLength();
    int element_buffer_length = getQuardTreeElementIndexLength() + getSkydomePosLength();
    using_vertex_offset = vertex_offset;
    vertex_offset_snap = using_vertex_offset;

    glm::detail::uint32* g_terrain_texture_array_data = new glm::detail::uint32[terrain_texture_size*terrain_texture_size];
    int terrain_texture_unit_id = 0;
    glActiveTexture(GL_TEXTURE0 + terrain_texture_unit_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLuint terrain_texture_id;
    glGenTextures(1, &terrain_texture_id);
    glBindTexture(GL_TEXTURE_2D, terrain_texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, terrain_texture_size, terrain_texture_size, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, g_terrain_texture_array_data);
    GLuint terrain_texure_uniform_id  = glGetUniformLocation(programID, "terrain_texture_sampler");
    glUniform1i(terrain_texure_uniform_id, terrain_texture_unit_id);

    glGenBuffers(1, &pixelBuffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(glm::detail::uint32)*terrain_texture_size*terrain_texture_size, g_terrain_texture_array_data, GL_STREAM_DRAW);
    delete[] g_terrain_texture_array_data;
    g_mapped_terrain_texture_array_data = (glm::detail::uint32*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, sizeof(glm::detail::uint32)*terrain_texture_size*terrain_texture_size, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT);

    int scatter_texture_unit_id = 1;
    glActiveTexture(GL_TEXTURE0 + scatter_texture_unit_id);
    GLuint scatter_texture_id;
    glGenTextures(1, &scatter_texture_id);
    glBindTexture(GL_TEXTURE_3D, scatter_texture_id);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, scatter_texture_size, scatter_texture_size, scatter_texture_size, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, g_scatter_texture_array_data);
    delete[] g_scatter_texture_array_data;
    GLuint scatter_texure_uniform_id  = glGetUniformLocation(programID, "scatter_texture_sampler");
    glUniform1i(scatter_texure_uniform_id, scatter_texture_unit_id);

    glm::detail::uint32* spacecube_texture_array_data = new glm::detail::uint32[spacecube_texture_size*spacecube_texture_size];
    fillSpacecubeTexture(spacecube_texture_array_data);
    int spacecube_texture_unit_id = 2;
    glActiveTexture(GL_TEXTURE0 + spacecube_texture_unit_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // ?
    GLuint spacecube_texture_id;
    glGenTextures(1, &spacecube_texture_id);
    glBindTexture(GL_TEXTURE_2D, spacecube_texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, spacecube_texture_size, spacecube_texture_size, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, spacecube_texture_array_data);
    delete[] spacecube_texture_array_data;
    GLuint spacecube_texure_uniform_id  = glGetUniformLocation(programID, "spacecube_texture_sampler");
    glUniform1i(spacecube_texure_uniform_id, spacecube_texture_unit_id);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffer);

    g_vertex_buffer_data = new glm::vec3*[2];
    g_vertex_buffer_data[0] = new glm::vec3[vertex_buffer_length];
    g_vertex_buffer_data[1] = new glm::vec3[vertex_buffer_length];
    glm::vec2* g_vertex_uv_data = new glm::vec2[getQuardTreeUVLength()];
    createSpaceCube(g_vertex_buffer_data[0], &vertex_static_data_length, g_vertex_uv_data);
    std::copy(&g_vertex_buffer_data[0][0], &g_vertex_buffer_data[0][vertex_static_data_length], g_vertex_buffer_data[1]);

    glGenBuffers(2, vertexbuffer);
    using_buffer_data = g_vertex_buffer_data[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertex_buffer_length, g_vertex_buffer_data[0], GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertex_buffer_length, g_vertex_buffer_data[1], GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[renderingBufferIndex]);
    g_mapped_vertex_buffer_data = (glm::vec3*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3)*vertex_buffer_length, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));

    glGenBuffers(2, uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*getQuardTreeUVLength(), g_vertex_uv_data, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*getQuardTreeUVLength(), g_vertex_uv_data, GL_STREAM_DRAW);
    delete[] g_vertex_uv_data;
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[renderingBufferIndex]);
    g_mapped_vertex_uv_data = (glm::vec2*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2)*getQuardTreeUVLength(), GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));

    glGenBuffers(2, normalbuffer);
    glm::vec3* g_vertex_normal_data = new glm::vec3[getQuardTreeNormalLength()];
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*getQuardTreeNormalLength(), g_vertex_normal_data, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*getQuardTreeNormalLength(), g_vertex_normal_data, GL_STREAM_DRAW);
    delete[] g_vertex_normal_data;
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[renderingBufferIndex]);
    g_mapped_vertex_normal_data = (glm::vec3*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3)*getQuardTreeNormalLength(), GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));

    glGenBuffers(2, elementBuffer);
    unsigned int* g_vertex_element_data = new unsigned int[element_buffer_length];
    glBindBuffer(GL_ARRAY_BUFFER, elementBuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int)*element_buffer_length, g_vertex_element_data, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, elementBuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int)*element_buffer_length, g_vertex_element_data, GL_STREAM_DRAW);
    delete[] g_vertex_element_data;
    glBindBuffer(GL_ARRAY_BUFFER, elementBuffer[renderingBufferIndex]);
    g_mapped_vertex_element_data = (unsigned int*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(unsigned int)*element_buffer_length, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));
    

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    updateData(false);
    unmapBuffers();

    std::thread first (updateData, true);
    first.detach();

    int update_frame_interval = 5;
    do{
        if (frame_counter%200 == update_frame_interval && updating == false && unmapping == false) {
            //use the buffers that is updated
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[renderingBufferIndex]);
            glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0,(void*)0 );
            glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[renderingBufferIndex]);
            glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0,(void*)0 );
            glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[renderingBufferIndex]);
            glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 0,(void*)0 );
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer[renderingBufferIndex]);
            glActiveTexture(GL_TEXTURE0 + terrain_texture_unit_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, terrain_texture_size, terrain_texture_size, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 0);

            //update vars
            elemant_index_terrain_length_rendering = elemant_index_terrain_length_update;
            elemant_index_sky_length_rendering = elemant_index_sky_length_update;
            viewPos = viewPos - vertex_offset_diff;
            using_buffer_data = g_vertex_buffer_data[renderingBufferIndex];
            using_vertex_offset = vertex_offset_snap;
            node_to_del = node;
            node = new_node;
            renderingBufferIndex = (renderingBufferIndex+1)%2;
        }
        if (frame_counter%200 == update_frame_interval*2 && updating == false && unmapping == false) {
            //orphaning and mapping the buffers that is not used and to be updated
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[renderingBufferIndex]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertex_buffer_length, NULL, GL_STREAM_DRAW);
            g_mapped_vertex_buffer_data = (glm::vec3*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3)*vertex_buffer_length, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));
            if (NULL == g_mapped_vertex_buffer_data) {
                std::cout << "Error: " << glGetError() << std::endl;
                throw std::runtime_error("Failed to map buffer.");
            }
            glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[renderingBufferIndex]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*getQuardTreeUVLength(), NULL, GL_STREAM_DRAW);
            g_mapped_vertex_uv_data = (glm::vec2*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2)*getQuardTreeUVLength(), GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));
            glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[renderingBufferIndex]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*getQuardTreeNormalLength(), NULL, GL_STREAM_DRAW);
            g_mapped_vertex_normal_data = (glm::vec3*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3)*getQuardTreeNormalLength(), GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));
            glBindBuffer(GL_ARRAY_BUFFER, elementBuffer[renderingBufferIndex]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int)*element_buffer_length, NULL, GL_STREAM_DRAW);
            g_mapped_vertex_element_data = (unsigned int*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(unsigned int)*element_buffer_length, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));

            glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(glm::detail::uint32)*terrain_texture_size*terrain_texture_size, NULL, GL_STREAM_DRAW);
            g_mapped_terrain_texture_array_data = (glm::detail::uint32*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, sizeof(glm::detail::uint32)*terrain_texture_size*terrain_texture_size, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT);
            updating = true;
        }
        frame_counter++;
        if (unmapping && frame_counter%200 == 0) {
            unmapBuffers();
        }

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        computeMatricesFromInputs();
        glm::mat4 M = glm::mat4(1.0f);
        glm::mat4 V = getViewMatrix();
        glm::mat4 P = getProjectionMatrix();
        glm::mat4 notrans_VP = P * V * glm::translate(viewPos);
        glm::mat4 MVP = P * V * M;
        glUniformMatrix4fv(matrixMVPID, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(matrixMID, 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(matrixVID, 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(matrix_notrans_VP_ID, 1, GL_FALSE, &notrans_VP[0][0]);
        glm::vec3 lightPos = 1000000.0f*calcFPosFromCoord(0.0f, -68.0f);
        glUniform3f(sun_worldspace_uniform_id, lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(vertex_offset_uniform_id, using_vertex_offset.x, using_vertex_offset.y, using_vertex_offset.z);
        glUniform1f(scatter_height_uniform_id, (glm::length(viewPos+using_vertex_offset)-earth_radius)/(atmosphere_top_radius-earth_radius));

        glDisable(GL_DEPTH_TEST);
        glUniform1i(render_target_uniform_id, 0);
        glDrawArrays(GL_TRIANGLES, 0, getSpaceCubePosLength());
        glEnable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glUniform1i(render_target_uniform_id, 1);
        glDrawElements(GL_TRIANGLE_STRIP, elemant_index_sky_length_rendering, GL_UNSIGNED_INT, reinterpret_cast<void*>((elemant_index_terrain_length_rendering+element_static_data_length)*4));
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glUniform1i(render_target_uniform_id, 2);
        glDrawElements(GL_TRIANGLE_STRIP, elemant_index_terrain_length_rendering, GL_UNSIGNED_INT, reinterpret_cast<void*>(element_static_data_length*4));

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( !glfwWindowShouldClose(window) );

    terminating = true;
    while(terminating) {
        std::this_thread::sleep_for (std::chrono::microseconds(100));
    }
    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    // Cleanup VBO
    glDeleteBuffers(2, vertexbuffer);
    glDeleteBuffers(2, uvbuffer);
    glDeleteBuffers(2, normalbuffer);
    glDeleteBuffers(2, elementBuffer);
    glDeleteVertexArrays(1, &VertexArrayID);

    return 0;
}

