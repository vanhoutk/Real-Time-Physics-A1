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
#include "Particle.h"
#include "Shader_Functions.h"
#include "time.h"

/*
 *	Globally defined variables and constants
 */
#define BUFFER_OFFSET(i) ((char *)NULL + (i))  // Macro for indexing vertex buffer

#define NUM_MESHES   1
#define NUM_SHADERS	 2
#define NUM_TEXTURES 1

using namespace std;

/*struct Particle {
	vec3 position;
	vec3 velocity;
	GLfloat mass;
	GLfloat life;
	vec3 force;
};*/

bool firstMouse = true;
bool keys[1024];
Camera camera(vec3(0.0f, 0.0f, 3.0f));
DWORD lastTime = 0;
enum Shaders { SKYBOX, SPHERE };
GLfloat cameraSpeed = 0.005f;
GLfloat deltaTime = 1.0f / 60.0f;
GLfloat lastX = 400, lastY = 300;
GLuint cubemapTexture;
GLuint lastUsedParticle = 0;
GLuint numParticles = 5000;
GLuint numNewParticles = 10;
GLuint shaderProgramID[NUM_SHADERS];
GLuint skyboxVAO, skyboxVBO;
GLuint sphereTextureID, sphereVAO;
int screenWidth = 1000;
int screenHeight = 800;
Mesh skybox;
Mesh sphere;
vector<Particle> particles;

// | Resource Locations
const char * meshFiles[NUM_MESHES] = { "../Meshes/sphere2.dae" };
const char * textureFiles[NUM_TEXTURES] = { "../Textures/particle.png" };
const char * vertexShaderNames[] = { "../Shaders/SkyboxVertexShader.txt", "../Shaders/ParticleVertexShader.txt"};
const char * fragmentShaderNames[] = { "../Shaders/SkyboxFragmentShader.txt", "../Shaders/ParticleFragmentShader.txt" };

void display() 
{
	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST);	// Enable depth-testing
	glDepthFunc(GL_LESS);		// Depth-testing interprets a smaller value as "closer"
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	glUseProgram(shaderProgramID[SPHERE]);
	glBindVertexArray(sphereVAO);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SPHERE], "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SPHERE], "proj"), 1, GL_FALSE, projection.m);
	
	for (Particle particle : particles)
	{
		if (particle.life > 0.0f)
		{
			mat4 model = identity_mat4();
			model = scale(model, vec3(0.005f, 0.005f, 0.005f));
			model = translate(model, particle.position);
			glUniform4fv(glGetUniformLocation(shaderProgramID[SPHERE], "colour"), 1, particle.colour.v);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SPHERE], "model"), 1, GL_FALSE, model.m);
			glDrawArrays(GL_TRIANGLES, 0, sphere.vertex_count);
		}
	}
	
	glutSwapBuffers();
}

void processInput()
{
	if (keys[GLUT_KEY_UP])
		camera.ProcessKeyboard(FORWARD, cameraSpeed);
	if(keys[GLUT_KEY_DOWN])
		camera.ProcessKeyboard(BACKWARD, cameraSpeed);
	if (keys[GLUT_KEY_LEFT])
		camera.ProcessKeyboard(LEFT, cameraSpeed);
	if (keys[GLUT_KEY_RIGHT])
		camera.ProcessKeyboard(RIGHT, cameraSpeed);
}

void processForces()
{
	for (GLuint i = 0; i < numParticles; ++i)
	{
		Particle &particle = particles[i];
		if (particle.life > 0.0f)
		{
			// Clear Forces
			particle.force = vec3(0.0f, 0.0f, 0.0f);

			// Apply Initial Force
			if ((particle.position.v[0] >= -0.05f && particle.position.v[0] <= 0.05f) &&
				(particle.position.v[2] >= -0.05f &&particle.position.v[2] <= 0.05f))
			{
				particle.force = vec3(particle.position.v[0] * 20.0f, 2.0f, particle.position.v[2] * 20.0f);
			}

			// Apply Gravity Force
			particle.force += vec3(0.0f, -0.981f, 0.0f);

			// Calculate the velocity
			particle.velocity += (particle.force / particle.mass) * deltaTime;

			// Calculate the position
			particle.position += particle.velocity * deltaTime;

			// Check for collision
			vec3 normal = vec3(0.0f, 1.0f, 0.0f);
			if ((dot((particle.position - vec3(0.0f, -1.0f, 0.0f)), normal) < 0.0f) && (dot(normal, particle.velocity) < 0.0f))
			{
				particle.position.v[1] = -1.0f;
				//particle.force.v[1] *= -0.8f;
				particle.velocity.v[1] *= -0.5f; // += (particle.force / particle.mass) * deltaTime;
				particle.position += particle.velocity * deltaTime;
			}
		}
	}

}

// https://learnopengl.com/#!In-Practice/2D-Game/Particles
GLuint FirstUnusedParticle()
{
	// Search from last used particle, this will usually return almost instantly
	for (GLuint i = lastUsedParticle; i < numParticles; i++) 
	{
		if (particles[i].life <= 0.0f) 
		{
			lastUsedParticle = i;
			return i;
		}
	}
	// Otherwise, do a linear search
	for (GLuint i = 0; i < lastUsedParticle; i++) 
	{
		if (particles[i].life <= 0.0f) 
		{
			lastUsedParticle = i;
			return i;
		}
	}
	// Override first particle if all others are alive
	lastUsedParticle = 0;
	return 0;
}

void RespawnParticle(Particle &particle)
{
	GLfloat randomX = ((rand() % 100) - 50) / 1000.0f;
	//GLfloat randomY = ((rand() % 100) - 50) / 0.75f;
	GLfloat randomY = 0.0f;
	GLfloat randomZ = ((rand() % 100) - 50) / 1000.0f;
	GLfloat randomRed = 0.0 + ((rand() % 15) / 100.0f);
	GLfloat randomGreen = 0.3 + ((rand() % 25) / 100.0f);
	GLfloat randomBlue = 0.5 + ((rand() % 50) / 100.0f);
	particle.position = vec3(randomX, randomY, randomZ);
	particle.colour = vec4(randomRed, randomGreen, 1.0f, 1.0f);
	particle.life = 5.0f;
	particle.velocity = vec3(0.0f, 0.0f, 0.0f);
}

void updateParticles()
{
	// Add new particles
	for (GLuint i = 0; i < numNewParticles; i++)
	{
		int unusedParticle = FirstUnusedParticle();
		RespawnParticle(particles[unusedParticle]);
	}
	// Uupdate all particles
	for (GLuint i = 0; i < numParticles; ++i)
	{
		Particle &p = particles[i];
		p.life -= deltaTime; // reduce life
		if (p.life > 0.0f)
		{	// particle is alive, thus update
			//p.position += p.velocity * deltaTime;
			p.colour.v[3] -= deltaTime * 2.5;
		}
	}
}

void updateScene()
{
	processInput();
	processForces();
	updateParticles();
	// Draw the next frame
	glutPostRedisplay();
}

void init()
{
	// Compile the shaders
	for (int i = 0; i < NUM_SHADERS; i++)
	{
		shaderProgramID[i] = CompileShaders(vertexShaderNames[i], fragmentShaderNames[i]);
	}
	//CompileShaders(NUM_SHADERS, shaderProgramID, vertexShaderNames, fragmentShaderNames);
	skybox.setupSkybox(&skyboxVAO, &skyboxVBO, &cubemapTexture);
	sphere = Mesh(&shaderProgramID[SPHERE]);
	sphere.generateObjectBufferMesh(sphereVAO, meshFiles[0]);
	//sphere.loadTexture(textureFiles[0], &sphereTextureID);

	for (GLuint i = 0; i < numParticles; i++)
		particles.push_back(Particle());
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