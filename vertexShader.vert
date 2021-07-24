#version 330 core
layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 vertexCol;

out vec3 vertexColor;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{

vertexColor = vertexCol;
gl_Position = projectionMatrix * viewMatrix * vec4(vertexPos.x, vertexPos.y, vertexPos.z, 1.0);
}