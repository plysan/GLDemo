#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <thread>
#include <chrono>
#include "tools/tools.hpp"
#include "tools/controls.hpp"
#include "tools/quardtree.hpp"

extern glm::vec3 viewPos;

GLFWwindow* window;
short windowW = 1366, windowH = 768;

bool updating = false;
bool unmapping = false;

glm::vec3* g_vertex_buffer_data;
glm::vec2* g_vertex_uv_data;
glm::vec3* g_vertex_normal_data;
unsigned int* g_vertex_element_data;
glm::detail::uint32* texture_array;
int quardTreeLength = 0;
int elemantIndexLength = 0;

int frameCounter = 0;

void key_callback(GLFWwindow* window, int key, int action)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void updateData(GLuint VertexArrayID, GLuint vertexbuffer, GLuint uvbuffer, GLuint normalbuffer, GLuint elementBuffer)
{
    while (true) {
        std::this_thread::sleep_for (std::chrono::seconds(3));
        if (!updating) {
            continue;
        }
        quardTreeLength=0;
        elemantIndexLength=0;
        createQuardTree(
            glm::vec2(19.0f, -157.0f),
            glm::vec2(21.0f, -155.0f),
            &quardTreeLength,
            g_vertex_buffer_data,
            g_vertex_uv_data,
            g_vertex_normal_data,
            &elemantIndexLength,
            g_vertex_element_data,
            texture_array
            );

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
    g_vertex_buffer_data = createQuardTreePos();
    g_vertex_uv_data = createQuardTreeUV();
    g_vertex_element_data = createQuardTreeElementIndex();
    g_vertex_normal_data = createNormal();
    texture_array = new glm::detail::uint32[2915*2915*4];
    clock_t before = clock();
    createQuardTree(
            glm::vec2(19.0f, -157.0f),
            glm::vec2(21.0f, -155.0f),
            &quardTreeLength,
            g_vertex_buffer_data,
            g_vertex_uv_data,
            g_vertex_normal_data,
            &elemantIndexLength,
            g_vertex_element_data,
            texture_array
            );
    printf("execution time: %fs\n", (double)(clock() - before)/CLOCKS_PER_SEC);
    printf("points: %d, indices: %d\n", quardTreeLength, elemantIndexLength);

    GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
    GLuint texture = readTextureFromArray(texture_array, 5830);

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertexBufferSize, NULL, GL_DYNAMIC_DRAW);

    GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*vertexBufferSize, NULL, GL_DYNAMIC_DRAW);

    GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertexBufferSize, NULL, GL_DYNAMIC_DRAW);

    GLuint elementBuffer;
    glGenBuffers(1, &elementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*ele_index_size, NULL, GL_DYNAMIC_DRAW);
    
    std::thread first (updateData, VertexArrayID, vertexbuffer, uvbuffer, normalbuffer, elementBuffer);
    first.detach();

    glUseProgram(programID);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0,(void*)0 );

    glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0,(void*)0 );

    glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 0,(void*)0 );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(TextureID, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

	do{
        if (frameCounter%500 == 0 && updating == false) {
            frameCounter++;
            printf("updating buf...\n");
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
            g_vertex_buffer_data = (glm::vec3*)(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE));
            glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
            g_vertex_uv_data = (glm::vec2*)(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE));
            glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
            g_vertex_normal_data = (glm::vec3*)(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
            g_vertex_element_data = (unsigned int*)(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE));
            if (NULL == g_vertex_buffer_data) {
                std::cout << "Error: " << glGetError() << std::endl;
                throw std::runtime_error("Failed to map buffer.");
            }
            updating = true;
        }
        if (updating) {
            std::this_thread::sleep_for (std::chrono::microseconds(10));
            continue;
        }
        frameCounter++;
        if (unmapping) {
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, elementBuffer);
            glUnmapBuffer(GL_ARRAY_BUFFER);
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
        glm::vec3 lightPos = 1000000.0f*calcPosFromCoord(20.0f, -158.0f);
        glUniform3f(LightPositionID, lightPos.x, lightPos.y, lightPos.z);

		//glDrawArrays(GL_POINTS, 0, quardTreeLength);
        glDrawElements(GL_TRIANGLE_STRIP, elemantIndexLength, GL_UNSIGNED_INT, (void*)0);

		// Swap buffers
		glfwSwapBuffers(window);
        glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( !glfwWindowShouldClose(window) );

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteVertexArrays(1, &VertexArrayID); 

	return 0;
}

