#version 410 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat4 vTransform[27];

out vec3 vsColor;
out vec3 vertPos;
out vec3 outNormal;

void main(void)
{
    mat4 trans = mvMatrix * vTransform[gl_VertexID/36];
    mat4 nTrans = inverse(trans);
    nTrans = transpose(nTrans);
    vec4 pos = trans * vec4(position, 1.0f);
    gl_Position = projMatrix * pos;
    outNormal = (trans * vec4(normal, 0.0f)).xyz;
    vsColor = color;
    vertPos = vec3(pos.xyz) / pos.w;
}
