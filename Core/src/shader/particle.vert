#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in mat4 model_instance;

//uniform mat4 model;
uniform mat4 PV;

void main()
{
    vec3 worldPos = vec3(model_instance * vec4(aPos, 1.0));
    gl_Position =  PV * vec4(worldPos, 1.0);
}