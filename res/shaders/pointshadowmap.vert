#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 instanceMatrix;

uniform mat4 model;
uniform bool useInstanceMatrix;

void main()
{
    mat4 finalModel = useInstanceMatrix ? instanceMatrix : model;

    gl_Position = finalModel * vec4(aPos, 1.0);
}  