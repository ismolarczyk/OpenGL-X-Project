#ifndef PARTICLE_HEADER_GUARD
#define PARTICLE_HEADER_GUARD

#ifndef SHADER_CODE_
// GLSL: Map glm types to native GLSL types

// C++: Include glm for vector types
#include <glm/glm.hpp>
using glm::vec3;
using glm::vec4;
using glm::vec2;
#endif

struct Particle {
	vec3 position;
	vec3 velocity;
	vec3 accel;
	vec4 color;
	vec2 scale;
	float life;
};



#endif