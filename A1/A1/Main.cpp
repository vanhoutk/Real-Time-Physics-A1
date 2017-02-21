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

using namespace std;

/*
 *	Globally defined variables and constants
 */
#define BUFFER_OFFSET(i) ((char *)NULL + (i))  // Macro for indexing vertex buffer

#define NUM_MESHES   2
#define NUM_SHADERS	 3
#define NUM_TEXTURES 3

bool firstMouse = true;
bool keys[1024];
Camera camera(vec3(0.0f, -0.5f, 3.0f));
enum Meshes { PARTICLE_MESH, PLANE_MESH };
enum Shaders { SKYBOX_SHADER, PARTICLE_SHADER, BASIC_TEXTURE_SHADER };
enum Textures { PARTICLE_TEXTURE, GROUND_TEXTURE, WALL_TEXTURE };
GLfloat cameraSpeed = 0.005f;
GLfloat deltaTime = 1.0f / 60.0f;
GLfloat friction = 0.98f;
GLfloat particleLifetime = 5.0f;
GLfloat restitution = 0.01f;
GLfloat snowGravityFactor = 0.1f;
GLuint lastUsedParticle = 0;
GLuint lastX = 400, lastY = 300;
GLuint numParticles = 30000;
GLuint numNewParticles = numParticles/((GLuint)particleLifetime *60);
GLuint shaderProgramID[NUM_SHADERS];
int screenWidth = 1000;
int screenHeight = 800;
Mesh skyboxMesh, particleMesh, groundMesh, wallMesh;
vec3 gravity = vec3(0.0f, -9.81f, 0.0f);
vec3 groundVector = vec3(0.0f, -1.0f, 0.0f);
vec3 groundNormal = vec3(0.0f, 1.0f, 0.0f);
vector<Particle> particles;

// | Resource Locations
const char * meshFiles[NUM_MESHES] = { "../Meshes/particle_reduced.dae", "../Meshes/plane.dae" };
const char * skyboxTextureFiles[6] = { "../Textures/DSposx.png", "../Textures/DSnegx.png", "../Textures/DSposy.png", "../Textures/DSnegy.png", "../Textures/DSposz.png", "../Textures/DSnegz.png"};
const char * textureFiles[NUM_TEXTURES] = { "../Textures/particle.png", "../Textures/asphalt.jpg", "../Textures/building.jpg" };

const char * vertexShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxVertexShader.txt", "../Shaders/ParticleVertexShader.txt", "../Shaders/BasicTextureVertexShader.txt" };
const char * fragmentShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxFragmentShader.txt", "../Shaders/ParticleFragmentShader.txt", "../Shaders/BasicTextureFragmentShader.txt" };

void display() 
{
	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST);	// Enable depth-testing
	glDepthFunc(GL_LESS);		// Depth-testing interprets a smaller value as "closer"
	glClearColor(5.0f/255.0f, 1.0f/255.0f, 15.0f/255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw skybox first
	mat4 view = camera.GetViewMatrix(); 
	mat4 projection = perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

	skyboxMesh.drawSkybox(view, projection);

	mat4 groundModel = identity_mat4();
	groundModel = scale(groundModel, vec3(2.0f, 1.0f, 20.0f));
	groundModel = translate(groundModel, vec3(0.0f, -1.0f, -5.0f));
	vec4 groundColour = vec4(0.5f, 1.0f, 1.0f, 1.0f);

	groundMesh.drawMesh(view, projection, groundModel);

	mat4 wallModel = identity_mat4();
	wallModel = scale(wallModel, vec3(20.0f, 1.0f, 2.0f));
	wallModel = rotate_y_deg(wallModel, 90.0f);
	wallModel = rotate_z_deg(wallModel, 90.0f);
	mat4 leftWallModel = translate(wallModel, vec3(-1.0f, 0.0f, -5.0f));
	mat4 rightWallModel = translate(wallModel, vec3(1.0f, 0.0f, -5.0f));

	wallMesh.drawMesh(view, projection, leftWallModel);
	wallMesh.drawMesh(view, projection, rightWallModel);
	
	for (Particle particle : particles)
	{
		if (particle.life > 0.0f)
		{
			mat4 particleModel = identity_mat4();
			particleModel = scale(particleModel, vec3(0.005f, 0.005f, 0.005f));
			particleModel = translate(particleModel, particle.position);

			particleMesh.drawMesh(view, projection, particleModel, particle.colour);
		}
	}
	
	glutSwapBuffers();
}

void updateForces(Particle &p)
{
	// Clear Forces
	p.force = vec3(0.0f, 0.0f, 0.0f);

	// Apply varying forces due to wind at different positions
	vec3 gust = vec3(0.0f, -0.5f, 1.5f);
	vec3 wind = vec3(0.015f, 0.0f, 0.0f);

	if (p.position.v[1] >= 1.2f && p.position.v[1] <= 1.5f)
	{
		p.force += wind;
	}
	else if (p.position.v[1] >= 0.6f)
	{
		p.force += wind * -2;
	}
	else if (p.position.v[1] >= 0.0f)
	{
		p.force += wind * 1;

		if (keys['w'])
			p.force += gust;
	}
	else if (p.position.v[1] >= -0.6f)
	{
		p.force += wind * -1;
	}


	// Apply Gravity Force
	p.force += gravity * snowGravityFactor * p.mass;
	// Ideally would calculate air resitance instead of using a gravity factor
}

GLuint firstUnusedParticle()
{
	// Start searching at the last unused particle
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

void respawnParticle(Particle &particle)
{
	GLfloat randomX = ((rand() % 1000) - 500) / 250.0f; // Snow
	GLfloat randomY = 1.5f;
	GLfloat randomZ = ((rand() % 1000) - 750) / 50.0f; // Snow

	vec4 white = vec4(1.0f, 1.0f, 1.0f, 0.75f);
	particle.position = vec3(randomX, randomY, randomZ);
	particle.colour = white;
	particle.life = particleLifetime; // ~5 Seconds
	particle.mass = 0.05f; 
	particle.velocity = vec3(0.0f, 0.0f, 0.0f);
}


void updateParticles()
{
	// Add new particles
	for (GLuint i = 0; i < numNewParticles; i++)
	{
		int unusedParticle = firstUnusedParticle();
		respawnParticle(particles[unusedParticle]);
	}


	// Update all particles
	for (GLuint i = 0; i < numParticles; ++i)
	{
		Particle &p = particles[i];

		// Update forces
		updateForces(p);

		// Update the velocity
		p.velocity += (p.force / p.mass) * deltaTime;

		// Update the position
		p.position += p.velocity * deltaTime;
		
		// Check for collision
		if ((dot((p.position - groundVector), groundNormal) < 0.0f) && (dot(groundNormal, p.velocity) < 0.0f))
		{
			vec3 velocityNormal = groundNormal * (dot(p.velocity, groundNormal));
			vec3 velocityTangent = p.velocity - velocityNormal;
			p.velocity = (velocityTangent * (1 - friction)) - (velocityNormal * restitution);
			p.position -= groundNormal * (2 * (dot((p.position - groundVector), groundNormal)));
		}

		// Apply evolution and recycling through the use of a life variable
		p.life -= deltaTime; // Reduce life
		if (p.life > 0.0f)
		{	
			// Change the colour of the particle (making it more transparent and less white)
			p.colour.v[0] -= deltaTime / 50.0f;
			p.colour.v[1] -= deltaTime / 50.0f;
			p.colour.v[2] -= deltaTime / 50.0f;
			p.colour.v[3] -= deltaTime / 5.0f;
		}
	}
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
	if (keys[(char)27])
		exit(0);
}

void updateScene()
{
	processInput();
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

	skyboxMesh = Mesh(&shaderProgramID[SKYBOX_SHADER]);
	skyboxMesh.setupSkybox(skyboxTextureFiles);

	groundMesh = Mesh(&shaderProgramID[BASIC_TEXTURE_SHADER]);
	groundMesh.generateObjectBufferMesh(meshFiles[PLANE_MESH]);
	groundMesh.loadTexture(textureFiles[1]);

	wallMesh = Mesh(&shaderProgramID[BASIC_TEXTURE_SHADER]);
	wallMesh.generateObjectBufferMesh(meshFiles[PLANE_MESH]);
	wallMesh.loadTexture(textureFiles[2]);

	particleMesh = Mesh(&shaderProgramID[PARTICLE_SHADER]);
	particleMesh.generateObjectBufferMesh(meshFiles[PARTICLE_MESH]);

	for (GLuint i = 0; i < numParticles; i++)
		particles.push_back(Particle());
}

/*
 *	User Input Functions
 */
#pragma region USER_INPUT_FUNCTIONS
void pressNormalKeys(unsigned char key, int x, int y)
{
	keys[key] = true;
}

void releaseNormalKeys(unsigned char key, int x, int y)
{
	keys[key] = false;
}

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

	int xoffset = x - lastX;
	int yoffset = lastY - y;

	lastX = x;
	lastY = y;

	camera.ProcessMouseMovement((GLfloat)xoffset, (GLfloat)yoffset);
}

void mouseWheel(int button, int dir, int x, int y)
{}
#pragma endregion

/*
 *	Main
 */
int main(int argc, char** argv) 
{
	srand((unsigned int)time(NULL));

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