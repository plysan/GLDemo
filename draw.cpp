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
#include "tools/quardtree.hpp"
#include "tools/physics.hpp"

extern glm::vec3 viewPos;
// to store viewPos upon quardtree update
glm::vec3 viewPos_cached;
// used to store vertex offset of the updating buffer and update to physics in the next buffer update
glm::vec3 vertex_offset_snap;

Node* new_node = NULL;

GLFWwindow* window;
short windowW = 1366, windowH = 768;

bool updating = false;
bool unmapping = false;
bool terminating = false;

glm::vec3** g_vertex_buffer_data;
glm::vec3* g_mapped_vertex_buffer_data;
glm::vec2* g_vertex_uv_data;
glm::vec2* g_mapped_vertex_uv_data;
glm::vec3* g_vertex_normal_data;
glm::vec3* g_mapped_vertex_normal_data;
unsigned int* g_vertex_element_data;
unsigned int* g_mapped_vertex_element_data;
glm::detail::uint32* texture_array;
glm::detail::uint32* mapped_texture_array;
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

void updateData()
{
    while (true) {
        std::this_thread::sleep_for (std::chrono::seconds(1));
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
        createQuardTree(
            glm::vec2(19.0f, -157.0f),
            glm::vec2(21.0f, -155.0f),
            &quardTreeLength,
            g_vertex_buffer_data[renderingBufferIndex],
            g_vertex_uv_data,
            g_vertex_normal_data,
            &elemantIndexLength,
            g_vertex_element_data,
            texture_array,
            &new_node
            );
        printf("execution time: %fs ", (double)(clock() - before)/CLOCKS_PER_SEC);
        printf("points: %d, indices: %d, nodes:%d\n", quardTreeLength, elemantIndexLength, nodeIndex);

        std::copy(&g_vertex_buffer_data[renderingBufferIndex][0], &g_vertex_buffer_data[renderingBufferIndex][quardTreeLength], g_mapped_vertex_buffer_data);
        std::copy(&g_vertex_uv_data[0], &g_vertex_uv_data[quardTreeLength], g_mapped_vertex_uv_data);
        std::copy(&g_vertex_normal_data[0], &g_vertex_normal_data[quardTreeLength], g_mapped_vertex_normal_data);
        std::copy(&g_vertex_element_data[0], &g_vertex_element_data[elemantIndexLength], g_mapped_vertex_element_data);
        std::copy(&texture_array[0], &texture_array[texture_dinmension*texture_dinmension], mapped_texture_array);

        unmapping = true;
        updating = false;
    }
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
    GLuint LightPositionID = glGetUniformLocation( programID, "LightPosition_worldspace" );

    setPosCoord(20.0f, -156.0f, 0.7f);
    using_vertex_offset = vertex_offset;
    vertex_offset_snap = using_vertex_offset;

    texture_array = new glm::detail::uint32[texture_dinmension*texture_dinmension];
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_dinmension, texture_dinmension, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, texture_array);

    GLuint pixelBuffer;
    glGenBuffers(1, &pixelBuffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(glm::detail::uint32)*texture_dinmension*texture_dinmension, texture_array, GL_STREAM_DRAW);

    GLuint* vertexbuffer = new GLuint[2];
    glGenBuffers(2, vertexbuffer);
    g_vertex_buffer_data = new glm::vec3*[2];
    g_vertex_buffer_data[0] = createQuardTreePos();
    g_vertex_buffer_data[1] = createQuardTreePos();
    using_buffer_data = g_vertex_buffer_data[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertexBufferSize, g_vertex_buffer_data[0], GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertexBufferSize, g_vertex_buffer_data[1], GL_STREAM_DRAW);

    GLuint* uvbuffer = new GLuint[2];
    glGenBuffers(2, uvbuffer);
    g_vertex_uv_data = createQuardTreeUV();
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*vertexBufferSize, g_vertex_uv_data, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*vertexBufferSize, g_vertex_uv_data, GL_STREAM_DRAW);

    GLuint* normalbuffer = new GLuint[2];
    glGenBuffers(2, normalbuffer);
    g_vertex_normal_data = createQuardTreeNormal();
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertexBufferSize, g_vertex_normal_data, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertexBufferSize, g_vertex_normal_data, GL_STREAM_DRAW);

    GLuint* elementBuffer = new GLuint[2];
    glGenBuffers(2, elementBuffer);
    g_vertex_element_data = createQuardTreeElementIndex();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*ele_index_size, g_vertex_element_data, GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*ele_index_size, g_vertex_element_data, GL_STREAM_DRAW);
    
    std::thread first (updateData);
    first.detach();

    glUseProgram(programID);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glActiveTexture(GL_TEXTURE0);
    GLuint TextureUID  = glGetUniformLocation(programID, "myTextureSampler");
    glUniform1i(TextureUID, 0);

    int update_frame_interval = 5;
    do{
        if (frameCounter%200 == update_frame_interval && updating == false && unmapping == false) {
            //use the buffers that is updated
            elemantIndexLengthForRendering = elemantIndexLength;
            glBindVertexArray(VertexArrayID);
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[renderingBufferIndex]);
            glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0,(void*)0 );
            glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[renderingBufferIndex]);
            glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0,(void*)0 );
            glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[renderingBufferIndex]);
            glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 0,(void*)0 );
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer[renderingBufferIndex]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_dinmension, texture_dinmension, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 0);

            viewPos = viewPos - viewPos_cached;
            using_buffer_data = g_vertex_buffer_data[renderingBufferIndex];
            using_vertex_offset = vertex_offset_snap;
            node = new_node;
            renderingBufferIndex = (renderingBufferIndex+1)%2;
        }
        if (frameCounter%200 == update_frame_interval*2 && updating == false && unmapping == false) {
            //orphaning and mapping the buffers that is not used and to be updated
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[renderingBufferIndex]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertexBufferSize, NULL, GL_STREAM_DRAW);
            g_mapped_vertex_buffer_data = (glm::vec3*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3)*vertexBufferSize, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));
            if (NULL == g_mapped_vertex_buffer_data) {
                std::cout << "Error: " << glGetError() << std::endl;
                throw std::runtime_error("Failed to map buffer.");
            }
            glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[renderingBufferIndex]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*vertexBufferSize, NULL, GL_STREAM_DRAW);
            g_mapped_vertex_uv_data = (glm::vec2*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2)*vertexBufferSize, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));
            glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[renderingBufferIndex]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertexBufferSize, NULL, GL_STREAM_DRAW);
            g_mapped_vertex_normal_data = (glm::vec3*)(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3)*vertexBufferSize, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer[renderingBufferIndex]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*ele_index_size, NULL, GL_STREAM_DRAW);
            g_mapped_vertex_element_data = (unsigned int*)(glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(unsigned int)*ele_index_size, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer[(renderingBufferIndex+1)%2]);

            glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(glm::detail::uint32)*texture_dinmension*texture_dinmension, NULL, GL_STREAM_DRAW);
            mapped_texture_array = (glm::detail::uint32*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, sizeof(glm::detail::uint32)*texture_dinmension*texture_dinmension, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT);
            updating = true;
        }
        frameCounter++;
        if (unmapping && frameCounter%200 == 0) {
            //unmapping the updated buffers
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[renderingBufferIndex]);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[renderingBufferIndex]);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[renderingBufferIndex]);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer[renderingBufferIndex]);
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

            unmapping = false;
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
        glUniform3f(LightPositionID, lightPos.x, lightPos.y, lightPos.z);

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

