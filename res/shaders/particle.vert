#version 460 core

struct Particle {
	vec3 position;
	vec3 velocity;
	vec3 accel;
	vec4 color;
	vec2 scale;
	float life;
};



layout(std430, binding = 0) readonly restrict buffer Particles {
    Particle particles[];
};

layout(location = 0) in vec2 aPos; // in [-0.5, 0.5]

layout(location = 0) uniform mat4 u_viewProj;
layout(location = 1) uniform vec3 u_cameraRight;
layout(location = 2) uniform vec3 u_cameraUp;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 vColor;

void main() {
    vTexCoord = aPos + 0.5;

    int index = gl_InstanceID;

    Particle particle = particles[index];

vec3 vertexPosition_worldspace =
    particle.position +
    u_cameraRight * aPos.x * particle.scale.x +
    u_cameraUp * aPos.y * particle.scale.y;


    gl_Position = u_viewProj * vec4(vertexPosition_worldspace, 1.0);
    vColor = particles[index].color;
}