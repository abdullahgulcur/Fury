#version 460 core

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D heightmap;
uniform usampler2D colormap;

//uniform vec3 color_d; // fucker lines
uniform vec3 lightDir;
uniform vec3 camPos;
//uniform vec3 lightColor;

uniform int mapSize;
uniform float triSize;

//uniform sampler2D textures[TEXTURE_SIZE];

//uniform sampler2DArray textureArray;

vec3 getNormalFromMap(vec3 normalColor, vec3 N)
{
    vec3 tangentNormal = normalColor * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

//vec3 getNormal(vec2 texcoord){
//
//    vec4 h;
//    h[0] = textureOffset(heightmap, texcoord, ivec2( 0,-1)).x; 
//	h[1] = textureOffset(heightmap, texcoord, ivec2(-1, 0)).x; 
//    h[2] = textureOffset(heightmap, texcoord, ivec2( 1, 0)).x; 
//    h[3] = textureOffset(heightmap, texcoord, ivec2( 0, 1)).x; 
//
//    vec3 n;
//    n.z = h[0] - h[3];
//    n.x = h[1] - h[2];
//    n.y = 2;
//
//    return normalize(n);
//}

void main(){

//    //FragColor = texture(textureArray, vec3(TexCoords.xy, 0));
////    int viewportX = 4;
////    int viewportY = 4;
////

//    vec2 texcood00 = vec2((WorldPos.x - mod(WorldPos.x, triSize)) / (mapSize * triSize), (WorldPos.z - mod(WorldPos.z, triSize)) /  (mapSize * triSize)); 
//    
//    unsigned int paletteIndex00 = texture(colormap, texcood00).r;
//    unsigned int paletteIndex10 = textureOffset(colormap, texcood00, ivec2(1, 0)).r; 
//    unsigned int paletteIndex01 = textureOffset(colormap, texcood00, ivec2(0, 1)).r; 
//    unsigned int paletteIndex11 = textureOffset(colormap, texcood00, ivec2(1, 1)).r;
//
//    vec3 albedo_00 = texture(textureArray, vec3(TexCoords.xy, paletteIndex00 * 2)).rgb;
//    vec3 normal_00 = texture(textureArray, vec3(TexCoords.xy, paletteIndex00 * 2 + 1)).rgb;
//
//    vec3 albedo_10 = texture(textureArray, vec3(TexCoords.xy, paletteIndex10 * 2)).rgb;
//    vec3 normal_10 = texture(textureArray, vec3(TexCoords.xy, paletteIndex10 * 2 + 1)).rgb;
//
//    vec3 albedo_01 = texture(textureArray, vec3(TexCoords.xy, paletteIndex01 * 2)).rgb;
//    vec3 normal_01 = texture(textureArray, vec3(TexCoords.xy, paletteIndex01 * 2 + 1)).rgb;
//
//    vec3 albedo_11 = texture(textureArray, vec3(TexCoords.xy, paletteIndex11 * 2)).rgb;
//    vec3 normal_11 = texture(textureArray, vec3(TexCoords.xy, paletteIndex11 * 2 + 1)).rgb;


//    float deltaX = mod(WorldPos.x, triSize) * (1 / triSize);
//    float deltaY = mod(WorldPos.z, triSize) * (1 / triSize);

//    vec3 color = albedo_00 * (1 - deltaX) * (1 - deltaY) + albedo_10 * deltaX * (1 - deltaY) + albedo_01 * (1 - deltaX) * deltaY + albedo_11 * deltaX * deltaY;
//    vec3 normal = normal_00 * (1 - deltaX) * (1 - deltaY) + normal_10 * deltaX * (1 - deltaY) + normal_01 * (1 - deltaX) * deltaY + normal_11 * deltaX * deltaY;
//
    //vec3 n = getNormalFromMap(normal.xyz, Normal);
    vec3 color = vec3(1,1,1);
    vec3 n = Normal;

    vec3 ambient = 0.25 * color;
    float diff = max(dot(lightDir, n), 0.0);
    vec3 diffuse = diff * color * 3;
    FragColor = vec4(ambient + diffuse, 1.0);
}