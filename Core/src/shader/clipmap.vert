#version 420 core

#define TERRAIN_INSTANCED_RENDERING

#ifdef TERRAIN_INSTANCED_RENDERING


layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 position_instance;
layout (location = 2) in vec2 clipmapcenter_instance;
layout (location = 3) in float level_instance;
layout (location = 4) in mat4 model_instance;

uniform mat4 PV;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

uniform sampler2DArray heightmapArray;
uniform float texSize;
uniform vec3 camPoss;
uniform vec3 camPos;

void main(void)
{
    vec3 p = vec3(model_instance * vec4(aPos, 1));
    float scale = pow(2, float(level_instance));
    vec3 pos = vec3(position_instance.x, 0, position_instance.y) + scale * p;
    WorldPos = pos;

    float terrainClipSize = scale * texSize;
    vec2 texCoords = mod(vec2(pos.x, pos.z), terrainClipSize);
    vec2 texCoords_ = mod(vec2(pos.x, pos.z), terrainClipSize * 2);
    texCoords /= terrainClipSize;
    texCoords_ /= terrainClipSize * 2;
    vec2 heightSample = texture(heightmapArray, vec3(texCoords.xy, level_instance)).rg;
    vec2 heightSample_ = texture(heightmapArray, vec3(texCoords_.xy, level_instance + 1)).rg;

    //pos.y = heightSample.r * 65.536f + heightSample.g * 0.256f;
    //pos.y *= 5;

    float factor = 10;
    float halfDist = (120 * 2 - 10) * scale;
    vec2 dist;
    dist.x = abs(pos.x - camPoss.x);
    dist.y = abs(pos.z - camPoss.z);
    vec2 ratio = dist / halfDist;
    vec2 num = ratio * factor;
    num = factor - num;
    num.x = clamp(num.x, 0.f, 1.f);
    num.y = clamp(num.y, 0.f, 1.f);
    float val = min(num.x, num.y);

    vec2 index0 = textureOffset(heightmapArray, vec3(texCoords.xy, level_instance), ivec2(0, -1)).rg;
    vec2 index1 = textureOffset(heightmapArray, vec3(texCoords.xy, level_instance), ivec2(-1, 0)).rg;
    vec2 index2 = textureOffset(heightmapArray, vec3(texCoords.xy, level_instance), ivec2(1, 0)).rg;
    vec2 index3 = textureOffset(heightmapArray, vec3(texCoords.xy, level_instance), ivec2(0, 1)).rg;

    vec2 index0_ = textureOffset(heightmapArray, vec3(texCoords_.xy, level_instance + 1), ivec2(0, -1)).rg;
    vec2 index1_ = textureOffset(heightmapArray, vec3(texCoords_.xy, level_instance + 1), ivec2(-1, 0)).rg;
    vec2 index2_ = textureOffset(heightmapArray, vec3(texCoords_.xy, level_instance + 1), ivec2(1, 0)).rg;
    vec2 index3_ = textureOffset(heightmapArray, vec3(texCoords_.xy, level_instance + 1), ivec2(0, 1)).rg;

    float h0_ = index0_.r * 65.536f + index0_.g * 0.256f;
    float h1_ = index1_.r * 65.536f + index1_.g * 0.256f;
    float h2_ = index2_.r * 65.536f + index2_.g * 0.256f;
    float h3_ = index3_.r * 65.536f + index3_.g * 0.256f;

    float h0 = index0.r * 65.536f + index0.g * 0.256f;
    float h1 = index1.r * 65.536f + index1.g * 0.256f;
    float h2 = index2.r * 65.536f + index2.g * 0.256f;
    float h3 = index3.r * 65.536f + index3.g * 0.256f;

    vec3 normal;
	normal.z = (h0 - h3) * 5;
	normal.x = (h1 - h2) * 5;
	normal.y = (2) * 5;
	normal = normalize(normal);

    vec3 normal_;
	normal_.z = (h0_ - h3_) * 5;
	normal_.x = (h1_ - h2_) * 5;
	normal_.y = (2) * 5;
	normal_ = normalize(normal_);

    vec3 T = normalize(vec3(2 * 5, (h1 - h2) * 5, 0));
    vec3 B = normalize(vec3(0, (h0 - h3) * 5, 2 * 5));
    TBN = transpose(mat3(T, B, normal));
    TangentViewPos = TBN * camPos;
    TangentFragPos = TBN * WorldPos;

    float height = (heightSample.r * 65.536f + heightSample.g * 0.256f) * 5;
    float height_ = (heightSample_.r * 65.536f + heightSample_.g * 0.256f) * 5;

    pos.y = val * height + (1 - val) * height_;
    vec3 nrml = val * normal + (1 - val) * normal_;
    Normal = normalize(nrml);

    TexCoords.x = WorldPos.x;
    TexCoords.y = WorldPos.z;
//    pos.y = val * 15;
//    Normal = vec3(0,1,0);

    //Normal = normal_;
    //pos.y = height_;

    gl_Position =  PV * vec4(pos, 1.0);
}


#endif


#ifndef TERRAIN_INSTANCED_RENDERING

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform vec2 texturePos;
uniform vec2 position;
uniform float scale;
uniform float texSize;
uniform int level;

uniform mat4 PV;
uniform sampler2DArray heightmapArray;

out vec3 WorldPos;
out vec2 TexCoords;
out vec3 Normal;

vec3 getNormal(vec2 texCoords){

    vec2 index0 = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, -1)).rg;
    vec2 index1 = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(-1, 0)).rg;
    vec2 index2 = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(1, 0)).rg;
    vec2 index3 = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, 1)).rg;

    float h0 = index0.r * 65.536f + index0.g * 0.256f;
    float h1 = index1.r * 65.536f + index1.g * 0.256f;
    float h2 = index2.r * 65.536f + index2.g * 0.256f;
    float h3 = index3.r * 65.536f + index3.g * 0.256f;

    vec3 normal;
	normal.z = h0 - h3;
	normal.x = h1 - h2;
	normal.y = 2;
	return normalize(normal);
}

float getHeightSetNormal(vec2 texCoords){

    vec2 tl = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(-1, -1)).rg;
    vec2 t  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, -1)).rg;
    vec2 tr = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(1, -1)).rg;
    vec2 l  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(-1, 0)).rg;
    vec2 m  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, 0)).rg;
    vec2 r  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(1, 0)).rg;
    vec2 bl = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(-1, 1)).rg;
    vec2 b  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, 1)).rg;
    vec2 br = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(1, 1)).rg;

    float h_tl = (tl.r * 65.536f + tl.g * 0.256f) * 2;
    float h_t  = (t.r  * 65.536f + t.g  * 0.256f) * 2;
    float h_tr = (tr.r * 65.536f + tr.g * 0.256f) * 2;
    float h_l  = (l.r  * 65.536f + l.g  * 0.256f) * 2;
    float h_m  = (m.r  * 65.536f + m.g  * 0.256f) * 2;
    float h_r  = (r.r  * 65.536f + r.g  * 0.256f) * 2;
    float h_bl = (bl.r * 65.536f + bl.g * 0.256f) * 2;
    float h_b  = (b.r  * 65.536f + b.g  * 0.256f) * 2;
    float h_br = (br.r * 65.536f + br.g * 0.256f) * 2;

 //0
//    float dX = h_tr + 2 * h_r + h_br - h_tl - 2 * h_l - h_bl;   
//	float dY = h_bl + 2 * h_b + h_br - h_tl - 2 * h_t - h_tr;
//
//	float normalStrength = 1.f;
//	Normal = normalize(vec3(dX, 2.0 / normalStrength, dY));

// 1
    vec3 cp_0 = cross(vec3(0,h_t-h_m,-1), vec3(-1,h_tl-h_m,0));
    vec3 cp_1 = cross(vec3(-1,h_tl-h_m,-1), vec3(-1,h_l-h_m,0));
    vec3 cp_2 = cross(vec3(-1,h_l-h_m,0), vec3(0,h_bl-h_m,1));
    vec3 cp_3 = cross(vec3(-1,h_bl-h_m,0), vec3(0,h_b-h_m,1));
    vec3 cp_4 = cross(vec3(0,h_b-h_m,1), vec3(1,h_br-h_m,0));
    vec3 cp_5 = cross(vec3(0,h_br-h_m,1), vec3(1,h_r-h_m,0));
    vec3 cp_6 = cross(vec3(1,h_r-h_m,0), vec3(0,h_tr-h_m,1));
    vec3 cp_7 = cross(vec3(1,h_tr-h_m,0), vec3(0,h_t-h_m,1));

    Normal = normalize(cp_0 + cp_1 + cp_2 + cp_3 + cp_4 + cp_5 + cp_6 + cp_7);

    return h_m;
}

//vec3 getNormal(vec2 texCoords){
//
//    vec2 index0 = textureOffset(heightmapArray, texCoords.xy, ivec2(0, -1)).rg;
//    vec2 index1 = textureOffset(heightmapArray, texCoords.xy, ivec2(-1, 0)).rg;
//    vec2 index2 = textureOffset(heightmapArray, texCoords.xy, ivec2(1, 0)).rg;
//    vec2 index3 = textureOffset(heightmapArray, texCoords.xy, ivec2(0, 1)).rg;
//
//    float h0 = index0.r * 65.536f + index0.g * 0.256f;
//    float h1 = index1.r * 65.536f + index1.g * 0.256f;
//    float h2 = index2.r * 65.536f + index2.g * 0.256f;
//    float h3 = index3.r * 65.536f + index3.g * 0.256f;
//
//    vec3 normal;
//	normal.z = h0 - h3;
//	normal.x = h1 - h2;
//	normal.y = 2;
//	return normalize(normal);
//}

void main(void)
{
    vec3 p = vec3(model * vec4(aPos, 1));
    vec3 pos = vec3(position.x, 0, position.y) + scale * p;
    vec2 texCoords = vec2(pos.x - texturePos.x, pos.z - texturePos.y) / texSize + 0.5f;

    //Normal = getHeightSetNormal(texCoords);
    //Normal = vec3(0,1,0);
    //vec2 heightSample = texture(heightmapArray, vec3(texCoords.xy, level)).rg;
    //vec2 heightSample = texture(heightmap, texCoords).rg;

    //float height = heightSample.r * 65.536f + heightSample.g * 0.256f;
    float height = getHeightSetNormal(texCoords);
    pos.y = height;
    WorldPos = pos;
    TexCoords = texCoords;
    gl_Position =  PV * vec4(pos, 1.0);
}

#endif
