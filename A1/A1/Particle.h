#pragma once

#include <GL/glew.h>
#include "Antons_maths_funcs.h"

class Particle{
public:
	vec3 position;
	vec3 velocity;
	GLfloat mass;
	GLfloat life;
	vec3 force;
	vec4 colour;

	Particle();
	Particle(vec3 position, vec3 velocity, GLfloat mass, GLfloat life, vec3 force, vec4 colour);
};

Particle::Particle()
{
	this->position = vec3(0.0f, 0.0f, 0.0f);
	this->velocity = vec3(0.0f, 0.0f, 0.0f);
	this->mass = 1.0f;
	this->life = 0.0f;
	this->force = vec3(0.0f, 0.0f, 0.0f);
}

Particle::Particle(vec3 position, vec3 velocity, GLfloat mass, GLfloat life, vec3 force, vec4 colour)
{
	this->position = position;
	this->velocity = velocity;
	this->mass = mass;
	this->life = life;
	this->force = force;
	this->colour = colour;
}