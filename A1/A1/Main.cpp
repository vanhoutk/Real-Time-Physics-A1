/*
 *	Includes
 */

#include <assimp/cimport.h>		// C importer
#include <assimp/scene.h>		// Collects data
#include <assimp/postprocess.h> // Various extra operations
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <math.h>
#include <mmsystem.h>
#include <stdio.h>
#include <vector>				// STL dynamic memory
#include <windows.h>

#include "Antons_maths_funcs.h" // Anton's maths functions
#include "time.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

 


/*
 *	Globally defined variables and constants
 */
#define BUFFER_OFFSET(i) ((char *)NULL + (i))  // Macro for indexing vertex buffer

#define NUM_MESHES 1
#define NUM_SHADERS 1

using namespace std;

GLuint shaderProgramID[NUM_SHADERS];
int width = 1000;
int height = 800;

/*
 *	Resource Locations
 */
const char * model_files[NUM_MESHES] = { "../Meshes/sphere.obj" };
const char * vertexShaderName[] = { "../Shaders/skybox.vert.txt", "../Shaders/phong.vert.txt.", "../Shaders/fresnel.vert.txt" };
const char * fragmentShaderName[] = { "../Shaders/skybox.frag.txt", "../Shaders/phong.frag.txt", "../Shaders/fresnel.frag.txt" };

struct Particle {
	vec3 position;
	vec3 velocity;
	GLfloat mass;
	GLfloat life;
	vec3 force;
};

/*
 *	Shader Functions
 */
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) 
{
	FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	for (int i = 0; i < NUM_SHADERS; i++) {
		shaderProgramID[i] = glCreateProgram();
		if (shaderProgramID[i] == 0) {
			fprintf(stderr, "Error creating shader program\n");
			exit(1);
		}
		// Create two shader objects, one for the vertex, and one for the fragment shader
		AddShader(shaderProgramID[i], vertexShaderName[i], GL_VERTEX_SHADER);
		AddShader(shaderProgramID[i], fragmentShaderName[i], GL_FRAGMENT_SHADER);

		GLint Success = 0;
		GLchar ErrorLog[1024] = { 0 };
		// After compiling all shader objects and attaching them to the program, we can finally link it
		glLinkProgram(shaderProgramID[i]);
		// check for program related errors using glGetProgramiv
		glGetProgramiv(shaderProgramID[i], GL_LINK_STATUS, &Success);
		if (Success == 0) {
			glGetProgramInfoLog(shaderProgramID[i], sizeof(ErrorLog), NULL, ErrorLog);
			fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
			exit(1);
		}

		// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
		glValidateProgram(shaderProgramID[i]);
		// check for program related errors using glGetProgramiv
		glGetProgramiv(shaderProgramID[i], GL_VALIDATE_STATUS, &Success);
		if (!Success) {
			glGetProgramInfoLog(shaderProgramID[i], sizeof(ErrorLog), NULL, ErrorLog);
			fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
			exit(1);
		}
		// Finally, use the linked shader program
		// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
		glUseProgram(shaderProgramID[i]);
	}

}
#pragma endregion

void display() 
{
	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST);	// Enable depth-testing
	glDepthFunc(GL_LESS);		// Depth-testing interprets a smaller value as "closer"
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glutSwapBuffers();
}

void processInput()
{}

void processForces()
{
	// For all particles
	// - Clear forces
	// - Apply each force in turn
	// - Solve for velocity and position
}

void updateScene()
{
	processInput();
	processForces();
	// Draw the next frame
	glutPostRedisplay();
}

void init()
{
	// Compile the shaders
	CompileShaders();


}

/*
 *	User Input Functions
 */
#pragma region USER_INPUT_FUNCTIONS
void pressNormalKeys(unsigned char key, int x, int y)
{}

void releaseNormalKeys(unsigned char key, int x, int y)
{}

void pressSpecialKeys(int key, int x, int y)
{}

void releaseSpecialKeys(int key, int x, int y)
{}

void mouseClick(int button, int state, int x, int y)
{}

void processMouse(int x, int y)
{}

void mouseWheel(int button, int dir, int x, int y)
{}
#pragma endregion

/*
 *	Main
 */
int main(int argc, char** argv) 
{
	srand(time(NULL));

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - width) / 2, (glutGet(GLUT_SCREEN_HEIGHT) - height) / 4);
	glutCreateWindow("Particle System");

	// Glut display and update functions
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);

	// User input functions
	glutKeyboardFunc(pressNormalKeys);
	glutKeyboardUpFunc(releaseNormalKeys);
	glutSpecialFunc(pressSpecialKeys);
	glutSpecialUpFunc(releaseSpecialKeys);
	glutMouseFunc(mouseClick);
	glutPassiveMotionFunc(processMouse);
	glutMouseWheelFunc(mouseWheel);


	glewExperimental = GL_TRUE; //for non-lab machines, this line gives better modern GL support
	
	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	// Set up meshes and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}