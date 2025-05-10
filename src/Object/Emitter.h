#pragma once

#include <glm/glm.hpp>
using glm::vec3, glm::vec4, glm::vec2;
#include "Shader.h"

struct ParticleEmitter {
	vec4 minColor, maxColor;
	vec3 minOffset, maxOffset;
	vec3 minVelocity, maxVelocity;
	vec3 minAccel, maxAccel;
	vec2 minScale, maxScale;
	float minLife, maxLife;
	// position of the emitter. In an engine you might prefer to use a transform component instead
	vec3 position;

	float spawnInterval, timer;
	int maxParticles;
	unsigned int particlesBuffer;
	unsigned freelistBuffer;

	unsigned sprite;

	void setUniformSettings(Shader& s);

	ParticleEmitter();
};
