#version 410 core

layout (location = 0) in vec3 position;
uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat4 vTransform[28];

void main(void)
{
    mat4 trans = mvMatrix * vTransform[gl_VertexID/36];
    gl_Position = projMatrix * trans * vec4(position, 1.0f);
}
