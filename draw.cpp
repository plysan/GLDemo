#include <stdio.h>
#include <iostream>
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
#include "tools/vars.hpp"

extern glm::vec3 viewPos;
// to store viewPos upon quardtree update
glm::vec3 viewPos_cached;
// used to store vertex offset of the updating buffer and update to physics in the next buffer update
glm::vec3 vertex_offset_snap;

Node* new_node = NULL;
Node* node_to_del = NULL;

GLFWwindow* window;
short windowW = 1366, windowH = 768;

bool updating = true;
bool unmapping = false;
bool terminating = false;

glm::vec3** g_vertex_buffer_data;
glm::vec3* g_mapped_vertex_buffer_data;
glm::vec2* g_mapped_vertex_uv_data;
glm::vec3* g_mapped_vertex_normal_data;
unsigned int* g_mapped_vertex_element_data;
glm::detail::uint32* g_mapped_terrain_texture_array_data;
int quardTreeLength = 0;
int elemantIndexLength = 0;
int elemantIndexLengthForRendering = 0;

int frameCounter = 205;
int renderingBufferIndex = 0;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
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
        quardTreeLength=0;
        elemantIndexLength=0;
        clock_t before = clock();
        viewPos_cached = viewPos;
        vertex_offset += viewPos_cached;
        vertex_offset_snap = vertex_offset;
        new_node = new Node;
        cleanupNode(&node_to_del);
        createQuardTree(
            glm::vec2(17.0f, -160.0f),
            glm::vec2(24.0f, -153.0f),
            &quardTreeLength,
            g_vertex_buffer_data[renderingBufferIndex],
            g_mapped_vertex_uv_data,
            g_mapped_vertex_normal_data,
            &elemantIndexLength,
            g_mapped_vertex_element_data,
            g_mapped_terrain_texture_array_data,
            &new_node
            );
        createSkydome(g_vertex_buffer_data[renderingBufferIndex], &quardTreeLength, g_mapped_vertex_element_data, &elemantIndexLength,
            glm::vec2(20.0f, -156.0f), 6, 1, 10);
        printf("execution time: %fs ", (double)(clock() - before)/CLOCKS_PER_SEC);
        printf("points: %d, indices: %d, nodes:%d\n", quardTreeLength, elemantIndexLength, nodeIndex);

        std::copy(&g_vertex_buffer_data[renderingBufferIndex][0], &g_vertex_buffer_data[renderingBufferIndex][quardTreeLength], g_mapped_vertex_buffer_data);

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

    window = glfwCreateWindow( windowW, windowH, "holly", NULL, NULL );
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
    glClearColor(0.0f, 0.0f, 0.1f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders( "tools/VertexShader", "tools/FragmentShader" );
    GLuint matrixMVPID = glGetUniformLocation( programID, "MVP" );
    GLuint matrixMID = glGetUniformLocation( programID, "M" );
    GLuint matrixVID = glGetUniformLocation( programID, "V" );
    GLuint sun_worldspace_uniform_id = glGetUniformLocation( programID, "sun_ws" );

    setPosCoord(20.0f, -156.0f, 0.7f);
    updateSkydomeConf(6, 10);
    int vertex_buffer_length = getQuardTreePosLength() + getSkydomePosLength();
    int element_buffer_length = getQuardTreeElementIndexLength() + getSkydomePosLength();
    using_vertex_offset = vertex_offset;
    vertex_offset_snap = using_vertex_offset;

    glm::detail::uint32* g_terrain_texture_array_data = new glm::detail::uint32[terrain_texture_size*terrain_texture_size];
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLuint terrain_texture_id;
    glGenTextures(1, &terrain_texture_id);
    glBindTexture(GL_TEXTURE_2D, terrain_texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, terrain_texture_size, terrain_texture_size, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, g_terrain_texture_array_data);

    glGenBuffers(1, &pixelBuffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(glm::detail::uint32)*terrain_texture_size*terrain_texture_size, g_terrain_texture_array_data, GL_STREAM_DRAW);
    delete[] g_terrain_texture_array_data;
    g_mapped_terrain_texture_array_data = (glm::detail::uint32*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, sizeof(glm::detail::uint32)*terrain_texture_size*terrain_texture_size, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT);

    glGenBuffers(2, vertexbuffer);
    g_vertex_buffer_data = new glm::vec3*[2];
    g_vertex_buffer_data[0] = new glm::vec3[vertex_buffer_length];
    g_vertex_buffer_data[1] = new glm::vec3[vertex_buffer_length];
    using_buffer_data = g_vertex_buffer_data[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertex_buffer_length, g_vertex_buffer_data[0], GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertex_buffer_length, g_vertex_buffer_data[1], GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[renderingBufferIndex]);
    g_mapped_vertex_buffer_data = (glm::vec3*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3)*vertex_buffer_length, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));

    glGenBuffers(2, uvbuffer);
    glm::vec2* g_vertex_uv_data = new glm::vec2[getQuardTreeUVLength()];
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
    
    glUseProgram(programID);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glActiveTexture(GL_TEXTURE0);
    GLuint terrain_texure_u_id  = glGetUniformLocation(programID, "terrain_texture_sampler");
    glUniform1i(terrain_texure_u_id, 0);

    updateData(false);
    unmapBuffers();

    std::thread first (updateData, true);
    first.detach();

    int update_frame_interval = 5;
    do{
        if (frameCounter%200 == update_frame_interval && updating == false && unmapping == false) {
            //use the buffers that is updated
            elemantIndexLengthForRendering = elemantIndexLength;
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[renderingBufferIndex]);
            glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0,(void*)0 );
            glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[renderingBufferIndex]);
            glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0,(void*)0 );
            glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[renderingBufferIndex]);
            glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 0,(void*)0 );
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer[renderingBufferIndex]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, terrain_texture_size, terrain_texture_size, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 0);

            viewPos = viewPos - viewPos_cached;
            using_buffer_data = g_vertex_buffer_data[renderingBufferIndex];
            using_vertex_offset = vertex_offset_snap;
            node_to_del = node;
            node = new_node;
            renderingBufferIndex = (renderingBufferIndex+1)%2;
        }
        if (frameCounter%200 == update_frame_interval*2 && updating == false && unmapping == false) {
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
        frameCounter++;
        if (unmapping && frameCounter%200 == 0) {
            unmapBuffers();
        }

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        computeMatricesFromInputs();
        glm::mat4 M = glm::mat4(1.0f);
        glm::mat4 V = getViewMatrix();
        glm::mat4 MVP = getProjectionMatrix() * V * M;
        glUniformMatrix4fv(matrixMVPID, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(matrixMID, 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(matrixVID, 1, GL_FALSE, &V[0][0]);
        glm::vec3 lightPos = 1000000.0f*calcFPosFromCoord(20.0f, -158.0f);
        glUniform3f(sun_worldspace_uniform_id, lightPos.x, lightPos.y, lightPos.z);

        //glDrawArrays(GL_POINTS, 0, quardTreeLength);
        glDrawElements(GL_TRIANGLE_STRIP, elemantIndexLengthForRendering, GL_UNSIGNED_INT, (void*)0);

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

