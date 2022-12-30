#version 420 core

layout (location = 0) in vec3 aPos;

uniform vec3 offsetMultPatchRes;
uniform int mapSize;
uniform mat4 PV;
uniform float scale;
uniform sampler2D heightmap;
uniform sampler2D normalmap;
uniform float triSize;

out vec3 WorldPos;
out vec2 TexCoords;
out vec3 Normal;

void main(void)
{
    vec3 pos = scale * (aPos + offsetMultPatchRes);
    vec2 texCoords = vec2(pos.x / mapSize, pos.z / mapSize);
    Normal = vec3(0,1,0);;//texture(normalmap, texCoords).rgb;
    float height = 0;//texture(heightmap, texCoords).r;
    pos.y = height;

    WorldPos = pos;
    vec2 pixelCoords = texCoords * mapSize + 0.5;
    texCoords = pixelCoords / mapSize;

    TexCoords = texCoords * mapSize * (1 / triSize);

    gl_Position =  PV * vec4(pos, 1.0);
}
