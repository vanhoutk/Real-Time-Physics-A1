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
#include "Mesh.h"
#include "Shader_Functions.h"
#include "time.h"

/*
 *	Globally defined variables and constants
 */
#define BUFFER_OFFSET(i) ((char *)NULL + (i))  // Macro for indexing vertex buffer

#define NUM_MESHES   1
#define NUM_SHADERS	 1
#define NUM_TEXTURES 1

using namespace std;

bool firstMouse = true;
bool keys[1024];
Camera camera(vec3(0.0f, 0.0f, 3.0f));
DWORD lastTime = 0;
enum Shaders { SKYBOX, SPHERE };
GLfloat deltaTime = 0.0f;
GLfloat lastX = 400, lastY = 300;
GLuint cubemapTexture;
GLuint shaderProgramID[NUM_SHADERS];
GLuint skyboxVAO, skyboxVBO;
GLuint sphereTextureID, sphereVAO;
int screenWidth = 1000;
int screenHeight = 800;
Mesh skybox;
Mesh sphere;

// | Resource Locations
const char * meshFiles[NUM_MESHES] = { "../Meshes/sphere.obj" };
const char * textureFiles[NUM_TEXTURES] = { "../Textures/particle.png" };
const char * vertexShaderNames[] = { "../Shaders/SkyboxVertexShader.txt", "../Shaders/ParticleVertexShader.txt"};
const char * fragmentShaderNames[] = { "../Shaders/SkyboxFragmentShader.txt", "../Shaders/ParticleFragmentShader.txt" };


/*struct mesh {
	GLuint num_vertices;
	vector<float> vertex_positions;
	vector<float> normals;
	vector<float> tex_coords;
};*/

struct Particle {
	vec3 position;
	vec3 velocity;
	GLfloat mass;
	GLfloat life;
	vec3 force;
};

void display() 
{
	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST);	// Enable depth-testing
	glDepthFunc(GL_LESS);		// Depth-testing interprets a smaller value as "closer"
	glClearColor(10.0f, 10.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw skybox first
	mat4 view = camera.GetViewMatrix(); // TODO: Figure out how to remove any translation component of the view matrix
	mat4 projection = perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

	glUseProgram(shaderProgramID[SKYBOX]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SKYBOX], "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SKYBOX], "proj"), 1, GL_FALSE, projection.m);
	
	glDepthMask(GL_FALSE);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);

	mat4 model = identity_mat4();

	glUseProgram(shaderProgramID[SPHERE]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SPHERE], "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SPHERE], "proj"), 1, GL_FALSE, projection.m);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SPHERE], "model"), 1, GL_FALSE, model.m);

	vec4 color = vec4(20.0f, 20.0f, 0.0f, 0.0f);

	glUniform4fv(glGetUniformLocation(shaderProgramID[SPHERE], "ParticleColor"), 1, color.v);
	
	glBindTexture(GL_TEXTURE_2D, sphereTextureID);
	glUniform1i(glGetUniformLocation(shaderProgramID[SPHERE], "TexCoords"), 0);

	glBindVertexArray(sphereVAO);

	glDrawArrays(GL_TRIANGLES, 0, sphere.vertex_count);

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
	CompileShaders(NUM_SHADERS, shaderProgramID, vertexShaderNames, fragmentShaderNames);
	skybox.setupSkybox(&skyboxVAO, &skyboxVBO, &cubemapTexture);
	sphere = Mesh(shaderProgramID[SPHERE]);
	sphere.generateObjectBufferMesh(sphereVAO, meshFiles[0]);
	sphere.loadTexture(textureFiles[0], &sphereTextureID);
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