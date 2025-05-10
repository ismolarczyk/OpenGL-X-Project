#version 460 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor; 

struct Material {
    sampler2D albedoMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D normalMap;
    sampler2D aoMap;
}; 

struct DirLight {
    vec3 direction;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

#define NR_POINT_LIGHTS 1

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in vec3 WorldPos;


uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;
uniform samplerCube depthMap;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

uniform float far_plane;

const float PI = 3.14159265359;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float DirectionalShadowCalculation(vec4 fragPosLightSpace);
float PointShadowCalculation(vec3 fragPos, vec3 N);

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(material.normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

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

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

vec3 calculateSpotLight(vec3 N, vec3 V, vec3 albedo, vec3 F0, float metallic, float roughness)
{
    vec3 L = normalize(spotLight.position - WorldPos);
    vec3 H = normalize(V + L);

    float distance = length(spotLight.position - WorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = spotLight.diffuse * attenuation;

    // Spotlight cone calculation
    float theta = dot(L, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    radiance *= intensity;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 calculateDirectionalLight(vec3 N, vec3 V, vec3 albedo, vec3 F0, float metallic, float roughness) {
    vec3 L = normalize(-dirLight.direction);  // Directional light vector
    vec3 H = normalize(V + L);               // Halfway vector

    // **1. Fresnel-Schlick Approximation**
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // **2. Microfacet Distribution (GGX)**
    float NDF = DistributionGGX(N, H, roughness);

    // **3. Geometry (Smith's Method)**
    float G = GeometrySmith(N, V, L, roughness);

    // **4. Specular BRDF**
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;
    
    // **5. Diffuse Contribution (Lambertian)**
    vec3 kS = F;                     // Specular amount
    vec3 kD = vec3(1.0) - kS;              // Diffuse amount
    kD *= 1.0 - metallic;            // Metals don’t diffuse

    vec3 diffuse = kD * albedo / PI;
    // **6. Final Radiance**
    vec3 radiance = dirLight.diffuse; // Light intensity
    vec3 Lo = (diffuse + specular) * radiance * NdotL;
    return Lo;
}



void main()
{   
    vec3 albedo     = pow(texture(material.albedoMap, TexCoords).rgb, vec3(2.2));
    float alpha     = texture(material.albedoMap,TexCoords).a;
    if(alpha<0.1){
    discard;
    }
    float metallic  = texture(material.metallicMap, TexCoords).r;
    float roughness = texture(material.roughnessMap, TexCoords).r;
    float ao        = texture(material.aoMap, TexCoords).r;
    vec3 N = getNormalFromMap();
    vec3 V = normalize(viewPos - WorldPos);
    vec3 R = reflect(-V, N); 
    FragColor = vec4(0);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    //F0 = vec3(1.0);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < NR_POINT_LIGHTS; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(pointLights[i].position - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(pointLights[i].position - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = pointLights[i].diffuse * attenuation;

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

        float shadow = PointShadowCalculation(FragPos, N)*1.5;
        FragColor += vec4(vec3(shadow),1);
        // for debug
        // add to outgoing radiance Lo
        Lo += (1.0-shadow) * (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   

    // Add spotlight contribution
    Lo += calculateSpotLight(N, V, albedo, F0, metallic, roughness);

// Add directional light contribution
    float shadow = DirectionalShadowCalculation(FragPosLightSpace)*1.5;

    Lo += (1.0-shadow) *  calculateDirectionalLight(N, V, albedo, F0, metallic, roughness);

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    irradiance = clamp(irradiance, 0.0, 1.0);
    vec3 diffuse      = irradiance * albedo;

    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 2.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb; 
    prefilteredColor = clamp(prefilteredColor,0.0,1.0);
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
    specular = clamp(specular,0.0,1.0);
    specular*=0.1;

    vec3 ambient    = (kD * diffuse + specular) * ao * 0.5; 

    
    vec3 color = ambient + Lo;
    color = 0.8 * color;

    // HDR tonemapping
    color = color / (color + vec3(1.0));

    // Gamma correction with higher gamma value
    color = pow(color, vec3(1.0 / 1.0)); // Darker gamma correction

    FragColor = vec4(color, 1.0);

    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(color, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}

float DirectionalShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)



    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow

    float bias = max(0.05 * (1.0 - dot(normalize(Normal), dirLight.direction)), 0.005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -2; x <= 2; ++x) // Expand the kernel
    {
        for(int y = -2; y <= 2; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 25.0; // Adjust for the increased sample count

    return shadow;
}  

float PointShadowCalculation(vec3 fragPos, vec3 N) {
    // Compute the direction from fragment to the light
    vec3 fragToLight = fragPos - pointLights[0].position;
    vec3 fragToLightDir = normalize(fragToLight);
    
    // Sample the depth map from the point light's cube map (depth cube map)
    float closestDepth = texture(depthMap, fragToLightDir).r;
    
    // Scale the closest depth value back to world units
    closestDepth *= far_plane;

    
    // Calculate the current fragment depth
    float currentDepth = length(fragToLight);
    FragColor = vec4(vec3(closestDepth / far_plane), 1.0);

    vec3 sampleOffsetDirections[20] = vec3[]
    (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );  

    float shadow = 0.0;
    float bias = max(0.05 * (1.0 - dot(normalize(fragToLight), N)), 0.005);

    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
    return shadow;
    
}

