#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include "tools/tools.hpp"
#include "tools/controls.hpp"
#include "tools/quardtree.hpp"

extern glm::vec3 viewPos;

GLFWwindow* window;
short windowW = 1366, windowH = 768;

void key_callback(GLFWwindow* window, int key, int action)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
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

    /*GLfloat* g_vertex_buffer_data = new GLfloat[25948806];
    for (int i=0; i<12974403; i++) {
        g_vertex_buffer_data[i*2] = ((GLfloat)(i%3603))/3601;
        g_vertex_buffer_data[i*2+1] = ((GLfloat)(i/3603))/3601;
    }*/
    setPosCoord(40.0f, 40.0f);
    int quardTreeLength = 0;
    glm::vec3* g_vertex_buffer_data = createQuardTreePos();
    glm::vec2* g_vertex_uv_data = createQuardTreeUV();
    clock_t before = clock();
    createQuardTree(
            glm::vec2(-90.0f, -180.0f),
            glm::vec2(90.0f, 180.0f),
            &quardTreeLength,
            g_vertex_buffer_data,
            g_vertex_uv_data
            );
    printf("execution time: %fs\n", (double)(clock() - before)/CLOCKS_PER_SEC);
    glm::vec3* g_vertex_normal_data = new glm::vec3[4];
    printf("points: %d\n", quardTreeLength);

    GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
    GLuint texture = readTiffImage("data/n33_w118_1arc_v2.tif");

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*quardTreeLength, g_vertex_buffer_data, GL_STATIC_DRAW);

    GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*quardTreeLength, g_vertex_uv_data, GL_STATIC_DRAW);

    GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*4, g_vertex_normal_data, GL_STATIC_DRAW);
    
	do{
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glUseProgram(programID);
        computeMatricesFromInputs();
        glm::mat4 M = glm::mat4(1.0f);
        glm::mat4 V = getViewMatrix();
        glm::mat4 MVP = getProjectionMatrix() * V * M;
        glUniformMatrix4fv(matrixMVPID, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(matrixMID, 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(matrixVID, 1, GL_FALSE, &V[0][0]);
        glUniform3f(LightPositionID, 0.0f, 9000000.0f, 0.0f);

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

        // Draw the triangle !
		glDrawArrays(GL_POINTS, 0, quardTreeLength);
        //printf("%d\n", quardTreeLength);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
        glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( !glfwWindowShouldClose(window) );

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

    delete [] g_vertex_buffer_data;

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteVertexArrays(1, &VertexArrayID); 

	return 0;
}

