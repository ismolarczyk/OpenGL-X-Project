#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 instanceMatrix; // Per-instance model matrix

uniform mat4 lightSpaceMatrix;
uniform mat4 model; // Regular model matrix
uniform bool useInstanceMatrix; // Flag to indicate instanced rendering

void main()
{
    mat4 finalModel = useInstanceMatrix ? instanceMatrix : model;
    gl_Position = lightSpaceMatrix * finalModel * vec4(aPos, 1.0);
}
