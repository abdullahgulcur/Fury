#version 460 core

struct Test {
    mat3 rotMatrix;
    vec3 normal;
    float height;
};


in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;
in vec3 TangentViewPos;
in vec3 TangentFragPos;
//in Test Wow;

out vec4 FragColor;

//uniform vec3 color_d;
uniform vec3 camPos;

// material parameters
//uniform sampler2D texture0;
//uniform sampler2D texture1;
//uniform sampler2D texture2;
//uniform sampler2D texture3;
//uniform sampler2D texture4;
//uniform sampler2D texture5;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

const float PI = 3.14159265359;

uniform float heightScale;

uniform sampler2DArray albedoArray;
uniform sampler2DArray normalArray;
uniform sampler2DArray maskArray;

//vec3 getNormalFromMap(vec3 normal)
//{
//    //vec3 tangentNormal = texture(texture1, TexCoords).xyz * 2.0 - 1.0;
//
////    vec3 Q1  = dFdx(WorldPos);
////    vec3 Q2  = dFdy(WorldPos);
////    vec2 st1 = dFdx(texCoords);
////    vec2 st2 = dFdy(texCoords);
////
////    vec3 N   = normalize(Normal);
////    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
////    vec3 B  = -normalize(cross(N, T));
////    mat3 TBN = mat3(T, B, N);
//
//    return normalize(TBN * tangentNormal);
//}

//////vec3 getNormalFromMap(vec2 texCoords)
//////{
//////    vec3 tangentNormal = texture(texture1, texCoords).xyz * 2.0 - 1.0;
//////
//////    return Wow.rotMatrix * tangentNormal;
//////}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

//#define parallax_occlusion
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir, float depth)
{  
#ifndef parallax_occlusion
    return texCoords - viewDir.xy * (depth * heightScale);        
#else      
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(maskArray, vec3(currentTexCoords.xy, 7)).a;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(maskArray, vec3(currentTexCoords.xy, 7)).a;  
        
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    
    float beforeDepth = texture(maskArray, vec3(prevTexCoords.xy, 7)).a - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
#endif
}

void main(){

//    vec3 view = camPos - WorldPos;
//    mat3 matrix = inverse(Wow.rotMatrix);
//    vec3 viewDir = normalize(matrix * view);
//    vec2 texCoords = TexCoords;
//    
//    texCoords = ParallaxMapping(texCoords, viewDir);
//
    
    //vec3 albedo = vec3(0.65f,0.3f,0.1f);//texture(texture0, texCoords).rgb;
//    float metallic = 0.1f;//texture(texture2, texCoords).r;
//    float roughness = 0.9f;//texture(texture3, texCoords).r;
//    float ao = 1.0f;//texture(texture4, texCoords).r;
    int index = int(Normal.y * 10);
    float height = texture(maskArray, vec3(TexCoords.xy, index)).a;

    // offset texture coordinates with Parallax Mapping
    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
    vec2 texCoords = ParallaxMapping(TexCoords, viewDir, height);

    vec3 albedo = pow(texture(albedoArray, vec3(texCoords.xy, index)).rgb, vec3(2.2));
    vec3 normal = texture(normalArray, vec3(texCoords.xy, index)).rgb * 2.0 - 1.0;;
    vec3 mask = texture(maskArray, vec3(texCoords.xy, index)).rgb;

    float metallic = mask.r;
    float roughness = mask.g;
    float ao = mask.b;

    // input lighting data
    vec3 N = TBN * normalize(normal);
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N); 

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    // calculate per-light radiance
    vec3 lightDir = vec3(1.f, -1.f, -1.f);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);
    float lightPow = 3.f;
    vec3 radiance = vec3(lightPow);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);    
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);        
        
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;	                
            
    // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);        

    // add to outgoing radiance Lo
    Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    
    // ambient lighting (we now use IBL as the ambient term)
    F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    kS = F;
    kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));
    //color *= ao;
    FragColor = vec4(color, 1.f);;
}
