#pragma once

// | Includes
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
#include "Shader_Functions.h"
#include "time.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

class Mesh {
public:
	int vertex_count;

	Mesh();
	Mesh(GLuint* shaderID);
	bool loadMesh(const char* file_name);
	void generateObjectBufferMesh(GLuint &vao, const char* file_name);
	bool loadTexture(const char* file_name, GLuint* texture_id);
	void setupSkybox(GLuint* skyboxVAO, GLuint* skyboxVBO, GLuint* cubemapTexture);
	GLuint loadCubemap(vector<const GLchar*> faces);
private:
	GLuint* shaderProgramID;
	
	vector<float> vertex_positions;
	vector<float> normals;
	vector<float> texture_coords;
};

Mesh::Mesh()
{}

Mesh::Mesh(GLuint* shaderID)
{
	shaderProgramID = shaderID;
}

bool Mesh::loadMesh(const char* file_name)
{
	const aiScene* scene = aiImportFile(file_name, aiProcess_Triangulate | aiProcess_CalcTangentSpace); // TRIANGLES!
	//fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return false;
	}
	printf("  %i animations\n", scene->mNumAnimations);
	printf("  %i cameras\n", scene->mNumCameras);
	printf("  %i lights\n", scene->mNumLights);
	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);

		vertex_positions.clear();
		normals.clear();
		texture_coords.clear();

		vertex_count = mesh->mNumVertices;


		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				//printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
				vertex_positions.push_back(vp->x);
				vertex_positions.push_back(vp->y);
				vertex_positions.push_back(vp->z);
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				//printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
				normals.push_back(vn->x);
				normals.push_back(vn->y);
				normals.push_back(vn->z);
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				//printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
				texture_coords.push_back(vt->x);
				texture_coords.push_back(vt->y);
			}
			if (mesh->HasTangentsAndBitangents()) {
				// NB: could store/print tangents here
			}
		}
	}

	aiReleaseImport(scene);
	return true;
}

void Mesh::generateObjectBufferMesh(GLuint &vao, const char* file_name) 
{
	// Load mesh and copy into buffers
	loadMesh(file_name);
	GLuint loc1 = glGetAttribLocation(*shaderProgramID, "vertex_position");
	GLuint loc2 = glGetAttribLocation(*shaderProgramID, "vertex_normal");
	GLuint loc3 = glGetAttribLocation(*shaderProgramID, "vertex_texture");

	unsigned int position_vbo = 0;
	glGenBuffers(1, &position_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), &vertex_positions[0], GL_STATIC_DRAW);
	unsigned int normal_vbo = 0;
	glGenBuffers(1, &normal_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, normal_vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), &normals[0], GL_STATIC_DRAW);
	unsigned int texture_vbo = 0;
	glGenBuffers(1, &texture_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, texture_vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 2 * sizeof(float), &texture_coords[0], GL_STATIC_DRAW);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, normal_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc3);
	glBindBuffer(GL_ARRAY_BUFFER, texture_vbo);
	glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindVertexArray(0);
}

bool Mesh::loadTexture(const char* file_name, GLuint* texture_id)
{
	int width, height, n;
	unsigned char* image = stbi_load(file_name, &width, &height, &n, STBI_rgb_alpha);
	if (!image) {
		fprintf(stderr, "ERROR: could not load %s\n", file_name);
		return false;
	}
	// NPOT check
	if ((width & (width - 1)) != 0 || (height & (height - 1)) != 0) {
		fprintf(stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name);
	}

	glGenTextures(1, texture_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

void Mesh::setupSkybox(GLuint* skyboxVAO, GLuint* skyboxVBO, GLuint* cubemapTexture)
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

	glGenVertexArrays(1, skyboxVAO);
	glGenBuffers(1, skyboxVBO);
	glBindVertexArray(*skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, *skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glBindVertexArray(0);

	vector<const GLchar*> faces;
	faces.push_back("../Textures/DSposx.png");
	faces.push_back("../Textures/DSnegx.png");
	faces.push_back("../Textures/DSposy.png");
	faces.push_back("../Textures/DSnegy.png");
	faces.push_back("../Textures/DSposz.png");
	faces.push_back("../Textures/DSnegz.png");
	*cubemapTexture = loadCubemap(faces);
}

// Loads a cubemap texture from 6 individual texture faces
// Order should be:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
GLuint Mesh::loadCubemap(vector<const GLchar*> faces)
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