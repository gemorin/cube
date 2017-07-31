#version 410 core

in vec3 vsColor;
in vec3 vertPos;
in vec3 outNormal;
in vec4 shadowCoord;
uniform int passThroughShader;
out vec4 color;

uniform sampler2DShadow shadowMap;

const vec3 lightPos = vec3(2.0,1.0,-5.0);
const vec3 diffuseColor = vec3(0.3, 0.3, 0.3);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 30.0;

vec2 poissonDisk[16] = vec2[](
   vec2( -0.94201624, -0.39906216 ),
   vec2( 0.94558609, -0.76890725 ),
   vec2( -0.094184101, -0.92938870 ),
   vec2( 0.34495938, 0.29387760 ),
   vec2( -0.91588581, 0.45771432 ),
   vec2( -0.81544232, -0.87912464 ),
   vec2( -0.38277543, 0.27676845 ),
   vec2( 0.97484398, 0.75648379 ),
   vec2( 0.44323325, -0.97511554 ),
   vec2( 0.53742981, -0.47373420 ),
   vec2( -0.26496911, -0.41893023 ),
   vec2( 0.79197514, 0.19090188 ),
   vec2( -0.24188840, 0.99706507 ),
   vec2( -0.81409955, 0.91437590 ),
   vec2( 0.19984126, 0.78641367 ),
   vec2( 0.14383161, -0.14100790 )
);

void main(void)
{
    float visibility=1.0;

    // Fixed bias, or...
    float bias = 0.005;

    // Sample the shadow map 4 times
    for (int i=0;i<4;i++){
        int index = i;
        // being fully in the shadow will eat up 4*0.2 = 0.8
        // 0.2 potentially remain, which is quite dark.
        visibility -= 0.2*(1.0-texture( shadowMap, vec3(shadowCoord.xy + poissonDisk[index]/700.0,  (shadowCoord.z-bias)/shadowCoord.w) ));
    }
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

        vec3 colorLinear = vsColor + ln * diffuseColor * visibility +
                           visibility * specular * specColor / 2.0;

        color.xyz = colorLinear;
    }
    else {
        color.xyz = vsColor * visibility;
    }
}
