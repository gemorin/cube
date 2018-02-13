#version 410 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat4 shadowMvp;
uniform mat4 vTransform[28];
uniform int passThroughShader;

out vec3 vsColor;
out vec3 vertPos;
out vec3 outNormal;
out vec4 shadowCoord;

const mat4 depthBias = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 0.5, 0.0,
0.5, 0.5, 0.5, 1.0);

void main(void)
{
    // shadowCoord = shadowMvp * vec4(position, 1.0);
    vsColor = color;
    if (passThroughShader == 0) {
        mat4 trans = mvMatrix * vTransform[gl_VertexID/36];
        shadowCoord = (depthBias * shadowMvp * vTransform[gl_VertexID/36]*vec4(position, 1.0));
        mat4 nTrans = inverse(trans);
        nTrans = transpose(nTrans);
        vec4 pos = trans * vec4(position, 1.0f);
        gl_Position = projMatrix * pos;
        outNormal = (nTrans * vec4(normal, 0.0f)).xyz;
        vertPos = vec3(pos.xyz) / pos.w;
    }
    else {
        shadowCoord = (depthBias * shadowMvp * vec4(position, 1.0));
        gl_Position = projMatrix * mvMatrix * vec4(position, 1.0f);
    }
}
