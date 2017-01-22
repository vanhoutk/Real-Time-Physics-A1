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

#define NUM_MESHES   2
#define NUM_SHADERS	 3
#define NUM_TEXTURES 3

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
enum Meshes { PARTICLE_MESH, PLANE_MESH };
enum Shaders { SKYBOX, PARTICLE_SHADER, BASIC_TEXTURE_SHADER };
GLfloat cameraSpeed = 0.005f;
GLfloat deltaTime = 1.0f / 60.0f;
GLfloat friction = 0.3f;
GLfloat lastX = 400, lastY = 300;
GLfloat resilience = 0.01f;
GLfloat snowGravityFactor = 0.1f;
GLuint cubemapTexture;
GLuint groundVAO, groundTextureID;
GLuint lastUsedParticle = 0;
GLuint numParticles = 30000;
GLuint numNewParticles = numParticles/(5*60);
GLuint shaderProgramID[NUM_SHADERS];
GLuint skyboxVAO, skyboxVBO;
GLuint particleTextureID, particleVAO;
GLuint wallTextureID;
int screenWidth = 1000;
int screenHeight = 800;
Mesh groundMesh;
Mesh skyboxMesh;
Mesh particleMesh;
vec3 gravity = vec3(0.0f, -9.81f, 0.0f);
vector<Particle> particles;

// | Resource Locations
const char * meshFiles[NUM_MESHES] = { "../Meshes/particle.dae", "../Meshes/plane.dae" };
const char * textureFiles[NUM_TEXTURES] = { "../Textures/particle.png", "../Textures/asphalt.jpg", "../Textures/building.jpg" };
const char * vertexShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxVertexShader.txt", "../Shaders/ParticleVertexShader.txt", "../Shaders/BasicTextureVertexShader.txt" };
const char * fragmentShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxFragmentShader.txt", "../Shaders/ParticleFragmentShader.txt", "../Shaders/BasicTextureFragmentShader.txt" };

void display() 
{
	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST);	// Enable depth-testing
	glDepthFunc(GL_LESS);		// Depth-testing interprets a smaller value as "closer"
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw skybox first
	mat4 view = camera.GetViewMatrix(); 
	mat4 projection = perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

	/*glUseProgram(shaderProgramID[SKYBOX]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SKYBOX], "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[SKYBOX], "proj"), 1, GL_FALSE, projection.m);
	
	glDepthMask(GL_FALSE);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);*/



	mat4 groundModel = identity_mat4();
	groundModel = scale(groundModel, vec3(2.0f, 1.0f, 20.0f));
	groundModel = translate(groundModel, vec3(0.0f, -1.0f, -5.0f));
	vec4 groundColour = vec4(0.5f, 1.0f, 1.0f, 1.0f);


	glBindVertexArray(groundVAO);
	glUseProgram(shaderProgramID[BASIC_TEXTURE_SHADER]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[BASIC_TEXTURE_SHADER], "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[BASIC_TEXTURE_SHADER], "projection"), 1, GL_FALSE, projection.m);

	glBindTexture(GL_TEXTURE_2D, groundTextureID);
	glUniform1i(glGetUniformLocation(shaderProgramID[BASIC_TEXTURE_SHADER], "basic_texture"), 0);
	//glUniform4fv(glGetUniformLocation(shaderProgramID[BASIC_TEXTURE_SHADER], "colour"), 1, groundColour.v);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[BASIC_TEXTURE_SHADER], "model"), 1, GL_FALSE, groundModel.m);
	glDrawArrays(GL_TRIANGLES, 0, groundMesh.vertex_count);

	mat4 wallModel = identity_mat4();
	wallModel = scale(wallModel, vec3(2.0f, 1.0f, 20.0f));
	wallModel = rotate_z_deg(wallModel, 90.0f);
	mat4 leftWallModel = translate(wallModel, vec3(-1.0f, 0.0f, -5.0f));
	mat4 rightWallModel = translate(wallModel, vec3(1.0f, 0.0f, -5.0f));
	glBindTexture(GL_TEXTURE_2D, wallTextureID);
	glUniform1i(glGetUniformLocation(shaderProgramID[BASIC_TEXTURE_SHADER], "basic_texture"), 0);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[BASIC_TEXTURE_SHADER], "model"), 1, GL_FALSE, leftWallModel.m);
	glDrawArrays(GL_TRIANGLES, 0, groundMesh.vertex_count);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[BASIC_TEXTURE_SHADER], "model"), 1, GL_FALSE, rightWallModel.m);
	glDrawArrays(GL_TRIANGLES, 0, groundMesh.vertex_count);
	
	glUseProgram(shaderProgramID[PARTICLE_SHADER]);
	glBindVertexArray(particleVAO);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[PARTICLE_SHADER], "view"), 1, GL_FALSE, view.m);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[PARTICLE_SHADER], "projection"), 1, GL_FALSE, projection.m);
	
	for (Particle particle : particles)
	{
		if (particle.life > 0.0f)
		{
			mat4 model = identity_mat4();
			model = scale(model, vec3(0.005f, 0.005f, 0.005f));
			model = translate(model, particle.position);
			glUniform4fv(glGetUniformLocation(shaderProgramID[PARTICLE_SHADER], "colour"), 1, particle.colour.v);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgramID[PARTICLE_SHADER], "model"), 1, GL_FALSE, model.m);
			glDrawArrays(GL_TRIANGLES, 0, particleMesh.vertex_count);
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
			vec3 groundVector = vec3(0.0f, -1.0f, 0.0f);
			vec3 groundNormal = vec3(0.0f, 1.0f, 0.0f);


			// Clear Forces
			particle.force = vec3(0.0f, 0.0f, 0.0f);

			// Apply Initial Force from Fountain
			/*if ((particle.position.v[0] >= -0.05f && particle.position.v[0] <= 0.05f) &&
				(particle.position.v[2] >= -0.05f &&particle.position.v[2] <= 0.05f))
			{
				particle.force = vec3(particle.position.v[0] * 20.0f, 2.0f, particle.position.v[2] * 20.0f);
			}*/

			
			
			// Apply the winds
			vec3 upper_wind = vec3(0.015f, 0.0f, 0.0f);
			//vec3 lower_wind = vec3(-0.05f, 0.0f, 0.0f);
			//vec3 circular_wind = cross(particle.position, normal) * 0.001;
			//circular_wind.v[1] = 0.0f;
			//particle.force += circular_wind;
			if (particle.position.v[1] >= 1.2f && particle.position.v[1] <= 1.5f)
			{
				particle.force += upper_wind;
			}
			else if (particle.position.v[1] >= 0.6f)
			{
				particle.force += upper_wind * -2;
			}
			else if (particle.position.v[1] >= 0.0f)
			{
				particle.force += upper_wind * 1;
			}
			else if (particle.position.v[1] >= -0.6f)
			{
				particle.force += upper_wind * -1;
			}

			// Apply Gravity Force
			//particle.force += vec3(0.0f, -0.981f, 0.0f) * particle.mass;
			particle.force += gravity * snowGravityFactor * particle.mass;
			// Ideally need to calculate air resitance

			// Calculate the velocity
			particle.velocity += (particle.force / particle.mass) * deltaTime;

			// Calculate the position
			particle.position += particle.velocity * deltaTime;
			

			// Check for collision
			if ((dot((particle.position - groundVector), groundNormal) < 0.0f) && (dot(groundNormal, particle.velocity) < 0.0f))
			{
				vec3 velocityNormal = groundNormal * (dot(particle.velocity, groundNormal));
				vec3 velocityTangent = particle.velocity - velocityNormal;
				particle.velocity = (velocityTangent * (1 - friction)) - (velocityNormal * resilience);
				particle.position -= groundNormal * (2 * (dot((particle.position - groundVector), groundNormal)));
				//particle.position -= particle.velocity * deltaTime;
				//particle.position.v[1] = -1.0f;
				//particle.force.v[1] *= -0.8f;
				//particle.velocity.v[1] *= -0.5f; // += (particle.force / particle.mass) * deltaTime;
				//particle.position += particle.velocity * deltaTime;
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
	//GLfloat randomX = ((rand() % 100) - 50) / 1000.0f; // Fountain
	GLfloat randomX = ((rand() % 1000) - 500) / 500.0f; // Snow
	//GLfloat randomY = ((rand() % 100) - 50) / 0.75f;
	GLfloat randomY = 1.5f;
	//GLfloat randomZ = ((rand() % 100) - 50) / 1000.0f; // Fountain
	GLfloat randomZ = ((rand() % 1000) - 750) / 50.0f; // Snow
	GLfloat randomRed = 0.0 + ((rand() % 15) / 100.0f);
	GLfloat randomGreen = 0.3 + ((rand() % 25) / 100.0f);
	GLfloat randomBlue = 0.5 + ((rand() % 50) / 100.0f);
	vec4 white = vec4(1.0f, 1.0f, 1.0f, 0.75f);
	particle.position = vec3(randomX, randomY, randomZ);
	//particle.colour = vec4(randomRed, randomGreen, 1.0f, 1.0f);
	particle.colour = white;
	particle.life = 5.0f;
	particle.mass = 0.05f; // Snow
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
			p.colour.v[3] -= deltaTime/5.0f;
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
	//skyboxMesh.setupSkybox(&skyboxVAO, &skyboxVBO, &cubemapTexture);
	groundMesh = Mesh(&shaderProgramID[BASIC_TEXTURE_SHADER]);
	groundMesh.generateObjectBufferMesh(groundVAO, meshFiles[PLANE_MESH]);
	groundMesh.loadTexture(textureFiles[1], &groundTextureID);
	groundMesh.loadTexture(textureFiles[2], &wallTextureID);

	particleMesh = Mesh(&shaderProgramID[PARTICLE_SHADER]);
	particleMesh.generateObjectBufferMesh(particleVAO, meshFiles[PARTICLE_MESH]);
	//particleMesh.loadTexture(textureFiles[0], &particleTextureID);

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