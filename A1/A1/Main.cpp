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
#include "Camera.h"
//#include "Shader.h"
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

bool firstMouse = true;
bool keys[1024];
Camera camera(vec3(0.0f, 0.0f, 3.0f));
DWORD lastTime = 0;
GLfloat deltaTime = 0.0f;
GLfloat lastX = 400, lastY = 300;
GLuint cubemapTexture;
GLuint shaderProgramID[NUM_SHADERS];
GLuint skyboxVAO, skyboxVBO;
int screenWidth = 1000;
int screenHeight = 800;

/*
 *	Resource Locations
 */
const char * model_files[NUM_MESHES] = { "../Meshes/sphere.obj" };
const char * vertexShaderNames[] = { "../Shaders/SkyboxVertexShader.txt"};
const char * fragmentShaderNames[] = { "../Shaders/SkyboxFragmentShader.txt" };
enum Shaders {SKYBOX};

//Shader skyboxShader("../Shaders/SkyboxVertexShader.txt", "../Shaders/SkyboxFragmentShader.txt");

// Function Prototypes
GLuint loadCubemap(vector<const GLchar*> faces);

struct Particle {
	vec3 position;
	vec3 velocity;
	GLfloat mass;
	GLfloat life;
	vec3 force;
};

void setupSkybox()
{
	GLfloat skyboxVertices[] = 
	{
		// Positions          
		-10.0f,  10.0f, -10.0f,
		-10.0f, -10.0f, -10.0f,
		 10.0f, -10.0f, -10.0f,
		 10.0f, -10.0f, -10.0f,
		 10.0f,  10.0f, -10.0f,
		-10.0f,  10.0f, -10.0f,

		-10.0f, -10.0f,  10.0f,
		-10.0f, -10.0f, -10.0f,
		-10.0f,  10.0f, -10.0f,
		-10.0f,  10.0f, -10.0f,
		-10.0f,  10.0f,  10.0f,
		-10.0f, -10.0f,  10.0f,

		10.0f, -10.0f, -10.0f,
		10.0f, -10.0f,  10.0f,
		10.0f,  10.0f,  10.0f,
		10.0f,  10.0f,  10.0f,
		10.0f,  10.0f, -10.0f,
		10.0f, -10.0f, -10.0f,

		-10.0f, -10.0f,  10.0f,
		-10.0f,  10.0f,  10.0f,
		 10.0f,  10.0f,  10.0f,
		 10.0f,  10.0f,  10.0f,
		 10.0f, -10.0f,  10.0f,
		-10.0f, -10.0f,  10.0f,

		-10.0f,  10.0f, -10.0f,
		 10.0f,  10.0f, -10.0f,
		 10.0f,  10.0f,  10.0f,
		 10.0f,  10.0f,  10.0f,
		-10.0f,  10.0f,  10.0f,
		-10.0f,  10.0f, -10.0f,

		-10.0f, -10.0f, -10.0f,
		-10.0f, -10.0f,  10.0f,
		 10.0f, -10.0f, -10.0f,
		 10.0f, -10.0f, -10.0f,
		-10.0f, -10.0f,  10.0f,
		 10.0f, -10.0f,  10.0f
	};

	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glBindVertexArray(0);

	vector<const GLchar*> faces;
	faces.push_back("../Textures/SkyboxRight.png");
	faces.push_back("../Textures/SkyboxLeft.png");
	faces.push_back("../Textures/SkyboxUp.png");
	faces.push_back("../Textures/SkyboxDown.png");
	faces.push_back("../Textures/SkyboxBack.png");
	faces.push_back("../Textures/SkyboxFront.png");
	cubemapTexture = loadCubemap(faces);
}

// Loads a cubemap texture from 6 individual texture faces
// Order should be:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
GLuint loadCubemap(vector<const GLchar*> faces)
{
	GLuint textureID;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &textureID);

	int width, height;
	unsigned char* image;

	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	for (GLuint i = 0; i < faces.size(); i++)
	{
		image = stbi_load(faces[i], &width, &height, 0, STBI_rgb);
		if (!image) {
			fprintf(stderr, "ERROR: could not load %s\n", faces[i]);
			return false;
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		free(image);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return textureID;
}

/*
 *	Shader Functions
 */
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) 
{
	FILE* fp;
	fopen_s(&fp, shaderFile, "r");

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
		AddShader(shaderProgramID[i], vertexShaderNames[i], GL_VERTEX_SHADER);
		AddShader(shaderProgramID[i], fragmentShaderNames[i], GL_FRAGMENT_SHADER);

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
	glClearColor(10.0f, 10.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw skybox first
	//glDepthMask(GL_FALSE);// Remember to turn depth writing off
	//skyboxShader.Use();
	mat4 view = camera.GetViewMatrix(); // TODO: Figure out how to remove any translation component of the view matrix
	mat4 projection = perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
	/*glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[0], "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[0], "projection"), 1, GL_FALSE, projection.m);*/
	
	int vLocation = glGetUniformLocation(shaderProgramID[0], "view");
	int pLocation = glGetUniformLocation(shaderProgramID[0], "proj");

	glUseProgram(shaderProgramID[0]);
	glUniformMatrix4fv(vLocation, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(pLocation, 1, GL_FALSE, projection.m);
	
	// skybox cube
	/*glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(shaderProgramID[0], "skybox"), 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthMask(GL_TRUE);*/

	glDepthMask(GL_FALSE);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);

	glutSwapBuffers();
}

void processInput()
{
	if(keys[GLUT_KEY_UP])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if(keys[GLUT_KEY_DOWN])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLUT_KEY_LEFT])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLUT_KEY_RIGHT])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void processForces()
{
	// For all particles
	// - Clear forces
	// - Apply each force in turn
	// - Solve for velocity and position
}

void updateScene()
{
	DWORD  current_time = timeGetTime();
	float  deltaTime = (current_time - lastTime) * 0.001f;
	lastTime = current_time;

	processInput();
	processForces();
	// Draw the next frame
	glutPostRedisplay();
}

void init()
{
	// Compile the shaders
	CompileShaders();
	setupSkybox();

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
{
	keys[key] = true;
}

void releaseSpecialKeys(int key, int x, int y)
{
	keys[key] = false;
}

void mouseClick(int button, int state, int x, int y)
{}

void processMouse(int x, int y)
{
	if (firstMouse)
	{
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	GLfloat xoffset = x - lastX;
	GLfloat yoffset = lastY - y;

	lastX = x;
	lastY = y;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

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
	glutInitWindowSize(screenWidth, screenHeight);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - screenWidth) / 2, (glutGet(GLUT_SCREEN_HEIGHT) - screenHeight) / 4);
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