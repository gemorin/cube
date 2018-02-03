#version 410 core

layout (location = 0) out vec4 color;

uniform sampler2D text;
in vec2 UV;

void main(void)
{
    float d = texture(text, UV).r;
    color = vec4(vec3(d), 1.0);
}
