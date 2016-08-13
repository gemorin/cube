#version 410 core

in vec3 vsColor;
in vec3 vertPos;
in vec3 outNormal;
uniform int passThroughShader;
out vec4 color;

const vec3 lightPos = vec3(2.0,1.0,-5.0);
const vec3 diffuseColor = vec3(0.3, 0.3, 0.3);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 30.0;

void main(void)
{
    if (passThroughShader == 0) {
        vec3 n = normalize(outNormal);
        vec3 l = normalize(lightPos - vertPos);

        float ln  = max(dot(l,n), 0.0);
        float specular = 0.0;

        if (ln > 0.0) {
            vec3 v = normalize(-vertPos);
            vec3 h = normalize(l + v);
            float specAngle = max(dot(h, n), 0.0);
            specular = pow(specAngle, shininess);
        }

        vec3 colorLinear = vsColor + ln * diffuseColor +
                           specular * specColor / 2.0;
        color.xyz = colorLinear;
    }
    else {
        color.xyz = vsColor;
    }
}
