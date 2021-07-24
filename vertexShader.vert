#version 330 core
layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 vertexCol;

out vec3 vertexColor;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

const float PI = 3.14;

void main()
{
gl_Position = projectionMatrix * viewMatrix * vec4(vertexPos, 1.0);
vertexColor = vertexCol;
}