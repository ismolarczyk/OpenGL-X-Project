#include "Emitter.h"
#include "Particle.h"
#include <glad/glad.h> 

#include <vector>
#include <algorithm>
#include <stb_image.h>

#include <filesystem>
#include <numeric>

void ParticleEmitter::setUniformSettings(Shader& s)
{


    // Emitter position (in world space)
    glm::vec3 position = { 0.0f, 0.2f, 0.0f };       // Centered at origin

    // Set color ranges
    s.setVec4("u_emitter.minColor", minColor);
    s.setVec4("u_emitter.maxColor", maxColor);

    // Set offset ranges
    s.setVec3("u_emitter.minOffset", minOffset);
    s.setVec3("u_emitter.maxOffset", maxOffset);

    // Set velocity ranges
    s.setVec3("u_emitter.minVelocity", minVelocity);
    s.setVec3("u_emitter.maxVelocity", maxVelocity);

    // Set acceleration ranges
    s.setVec3("u_emitter.minAccel", minAccel);
    s.setVec3("u_emitter.maxAccel", maxAccel);

    // Set scale ranges
    s.setVec2("u_emitter.minScale", minScale);
    s.setVec2("u_emitter.maxScale", maxScale);

    // Set lifetime ranges
    s.setFloat("u_emitter.minLife", minLife);
    s.setFloat("u_emitter.maxLife", maxLife);

    // Set emitter position
    s.setVec3("u_emitter.position", position);

    // SSBO

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->particlesBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->freelistBuffer);


}


ParticleEmitter::ParticleEmitter()
{

    minColor = { 0.1f, 0.1f, 0.1f, 1.0f };  // Dark gray, fully opaque
    maxColor = { 1.0f, 1.0f, 1.0f, 1.0f };  // White, fully opaque

    minOffset = { -0.05f, 0.0f, -0.05f };  // Small negative offsets
    maxOffset = { 0.05f, 0.2f, 0.05f };   // Small positive offsets

    minOffset *= 3;
    maxOffset *= 3;

    minVelocity = { -0.1f, -0.1f, -0.1f };  // Slow movement
    maxVelocity = { 0.1f, 0.1f, 0.1f };    // Small positive movement

    minAccel = { 0.0f, -0.01f, 0.0f };  // Subtle gravity-like acceleration
    maxAccel = { 0.0f, -0.02f, 0.0f };  // Slightly more gravity

    minScale = { 0.01f, 0.01f };   // Much smaller size
    maxScale = { 0.05f, 0.05f };   // Maximum size is 1.0

    // and values
    spawnInterval = 2.0;
    maxLife = 30;
    minLife = 15;
    maxParticles = 50;
    timer = 0;

	glCreateBuffers(1, &particlesBuffer);
	glCreateBuffers(1, &freelistBuffer);

    // sizeof Particle but with padding for gpu
	glNamedBufferStorage(particlesBuffer,
		80 * maxParticles,
		nullptr,
		GL_DYNAMIC_STORAGE_BIT);


	glNamedBufferStorage(freelistBuffer,
		sizeof(int) * (maxParticles+1),
		nullptr,
		GL_DYNAMIC_STORAGE_BIT);

   glNamedBufferSubData(freelistBuffer, 0, sizeof(int), &maxParticles);
   std::vector<int> indices(maxParticles);
   std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ..., maxParticles-1
   glNamedBufferSubData(freelistBuffer, sizeof(int), sizeof(int) * maxParticles, indices.data());





    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    unsigned char* data = stbi_load("res/models/TestScene/particle.png", &width, &height, &nrComponents, 0);


    if (data)
    {
        glGenTextures(1, &sprite);
        glBindTexture(GL_TEXTURE_2D, sprite);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." << std::endl;
    }

}
