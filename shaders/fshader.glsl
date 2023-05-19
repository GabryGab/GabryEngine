#version 460 core
#define MAX_LIGHTS 8

layout (binding = 0) uniform sampler2D diffuseSampler;
layout (binding = 1) uniform sampler2D specularSampler;
layout (binding = 2) uniform sampler2D normalsSampler;
layout (binding = 3) uniform sampler2D roughnessSampler;
layout (binding = 4) uniform sampler2D metallicSampler;
layout (binding = 8) uniform sampler2DArray shadowSampler;

in vec2 texCoords;
in vec3 Normal;
in vec3 FragPos;
in vec3 FragPosWorldSpace;
in mat3 TBN;

out vec4 FragColor;

struct SunLight {
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct Light {
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
    float linear;
    float quadratic;
};

struct Material {
	// Phong.
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;

	// PBR.
	float roughness, metallic;
	bool hasDiffuseMap, hasSpecularMap, hasNormalsMap, hasRoughnessMap, hasMetallicMap;
};

uniform bool useTexture;
uniform bool useLighting;
uniform Light lights[MAX_LIGHTS];
uniform int lightsNumber;
uniform Material material;
uniform mat4 view;
uniform vec3 defaultColor;
uniform SunLight sunLight;
uniform bool useSunLight;
uniform bool usePbr;
uniform bool useShadows, usePcf, usePoissonPcf;
uniform int pcfSamplesNumber;
uniform vec2 pcfSamples[16]; // 16 is the maximum number of pcf samples allowed.
uniform float poissonPcfDiameter;
uniform mat4 lightSpaceMatrices[4];
uniform float pcfMultipliers[4];
uniform float cascadePlaneDistances[5];
uniform float csmBlendingOffset;
uniform float shadowBiasMultiplier, shadowBiasMinimum;

vec3 calculateLightingBlinnPhong(vec3, vec3, vec3);
float calculateDiffuse(vec3, vec3);
float calculateSpecular(vec3, vec3);
float calculateShadow(vec3, vec3);

vec3 calculateLightingPbr();
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

float random(vec2 co);
vec2 rotatePcfPoint(vec2, float);

const float PI = 3.14159265359;

void main()
{	
	if(!useLighting) {
		FragColor = vec4(defaultColor, 1.0f);
		return;
	}
	vec3 lighting;
	if(usePbr) {
		lighting = calculateLightingPbr();
	} else {
		lighting = calculateLightingBlinnPhong(material.ambient, material.diffuse, material.specular);
	}
	FragColor = vec4(lighting, 1.0f);
}

vec3 calculateLightingPbr() {
	
	// If lighting doesn't look 100% right it's probably not this.
	// It usually is the model normals, not the lighting done here.
	// Set FragColor to vec4(Normal, 1.0f) to test if normals match weird lighting.
	// If they do match it's probably the normals.

	// Material properties.
	vec3 albedo = material.diffuse;
	if(useTexture && material.hasDiffuseMap)
		albedo = texture(diffuseSampler, texCoords).rgb;

	float roughness = material.roughness;
	if(material.hasRoughnessMap)
		roughness = texture(roughnessSampler, texCoords).r;

	float metallic = material.metallic;
	if(material.hasMetallicMap)
		metallic = texture(metallicSampler, texCoords).r;
	

	vec3 N = normalize(Normal);
	if(material.hasNormalsMap) {
		N = texture(normalsSampler, texCoords).rgb;
		N = normalize(N * 2.0 - 1.0);
		N = normalize(TBN * N);
	}

	vec3 V = normalize(-FragPos);

	vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);

	if(useSunLight) {
		 // calculate per-light radiance
        vec3 L = normalize(vec3(mat3(view) * sunLight.position));
        vec3 H = normalize(V + L);
        vec3 radiance     = sunLight.diffuse;    
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);   
		// This function generates dark edge when pixel is at 90 degrees
		// with view vector. This is because pixel is completely "occluded"
		// by geometry, but this makes it look odd,
		// because it's impossible in real life.
		// Check function to see how to fix it.
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);    
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    =  NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
        
		float shadowValue = 0.0f; 
		if(useShadows)
			shadowValue = calculateShadow(N, L);

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL * (1-shadowValue); 
	}

    for(int i = 0; i < lightsNumber; ++i)
    {
        // calculate per-light radiance
        vec3 L = vec3(view * vec4(lights[i].position, 1.0f)) - FragPos;
		float dis         = length(L);
		L	   = normalize(L);
        vec3 H = normalize(V + L);
        float attenuation = 1 / (lights[i].constant + lights[i].linear * dis + 
			lights[i].quadratic * (dis * dis));
        vec3 radiance     = lights[i].diffuse * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;

	return color;
} 

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
	// This is the part that creates dark edge.
	// If you want to get rid of dark edge, limit the dot product
	// to an angle decided by you, to avoid pixels being at 90 degrees
	// to the view vector which is impossible theoretically speaking.
	// It's impossible because if it was at 90 degrees in real life,
	// we wouldn't be able to actually see that pixel.

	// Replace <degrees> with theoretical maximum angle accepted in degrees.
	// float ggx2  = GeometrySchlickGGX(max(NdotV, cos(radians(<degrees>))), roughness);
	// Example:
	// float ggx2  = GeometrySchlickGGX(max(NdotV, cos(radians(89.995f))), roughness);

	// Be careful not to set the degrees to a too low value as this 
	// would make the fresnel effect too powerful, making pixels white.
	// Either find a good value, or change fresnel function
	// by also limiting the angle or final value there.

	// Everything written before this comment might just be a normal, as in 3D normals, problem.
	// Not solveable unless you use flat shading which is not always wanted.
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

vec3 calculateLightingBlinnPhong(vec3 ambient, vec3 diffuse, vec3 specular) {
	
	vec3 texDiffuse = vec3(1.0f, 1.0f, 1.0f);
	if(useTexture && material.hasDiffuseMap)
		texDiffuse = texture(diffuseSampler, texCoords).rgb;

	vec3 N = normalize(Normal);
	if(material.hasNormalsMap) {
		N = texture(normalsSampler, texCoords).rgb;
		N = normalize(N * 2.0 - 1.0);
		N = normalize(TBN * N);
	}
	vec3 V = normalize(-FragPos);

	vec3 amb = vec3(0,0,0);
	vec3 diff = vec3(0,0,0);
	vec3 spec = vec3(0,0,0);

	if(useSunLight) {
		vec3 lightDir = normalize(vec3(mat3(view) * sunLight.position));
		float shadowValue = 0.0f; 
		if(useShadows)
			shadowValue = calculateShadow(N, lightDir);
		amb+=sunLight.ambient * ambient;
		float localDiff = calculateDiffuse(N, lightDir);
		vec3 halfwayDir = normalize(V + lightDir);
		if(localDiff>0) 
			spec+= sunLight.specular * specular * calculateSpecular(N, halfwayDir) * (1-shadowValue);
		diff+= sunLight.diffuse * texDiffuse * diffuse * localDiff * (1-shadowValue);
	}

	for (int i = 0; i < lightsNumber; i++) {
		vec3 lightDir = vec3(view * vec4(lights[i].position, 1.0f)) - FragPos;
		float dis = length(lightDir);
		lightDir = normalize(lightDir);
		vec3 halfwayDir = normalize(V + lightDir);
		float attenuation = 1 / (lights[i].constant + lights[i].linear * dis + 
		lights[i].quadratic * (dis * dis));
		amb+= lights[i].ambient * ambient * attenuation;
		float localDiff = calculateDiffuse(N, lightDir);
		// Only add specular if diffuse > 0.
		if(localDiff > 0)
			spec += lights[i].specular * calculateSpecular(N, halfwayDir) * specular * attenuation;
		diff += lights[i].diffuse * texDiffuse * localDiff * diffuse * attenuation;
	}
	return (amb + diff + spec);
}

float calculateDiffuse(vec3 N, vec3 lightDir) {
	return max(dot(N, lightDir), 0.0);
}

float calculateSpecular(vec3 N, vec3 halfwayDir) {
	return pow(max(dot(N, halfwayDir), 0.0f), material.shininess);
}

float calculateShadowAtLayer(int layer, vec3 normal, vec3 lightDir);
float calculatePcfShadow(vec3 projCoords, vec2 texelSize, int layer, float currentDepth, float bias);
float calculatePoissonPcfShadow(vec3 projCoords, vec2 texelSize, int layer, float currentDepth, float bias);

float calculateShadow(vec3 normal, vec3 lightDir)
{	
	float depthValue = abs(FragPos.z);
    bool interpolate = false;
	int layer = -1;
	for (int i = 0; i < 3; ++i) {
		if (depthValue < cascadePlaneDistances[i+1] - csmBlendingOffset/2) {
			layer = i;
			break;
		} else if(depthValue < cascadePlaneDistances[i+1] + csmBlendingOffset/2) {
			layer = i;
			interpolate = true;
			break;
		}
	}
	if(layer == -1)
		layer = 3;

    float shadow = 0.0;

	shadow = calculateShadowAtLayer(layer, normal, lightDir);
	if(interpolate) {
		float shadow2 = calculateShadowAtLayer(layer+1, normal, lightDir);
		shadow = mix(shadow, shadow2, (depthValue-cascadePlaneDistances[layer+1]+csmBlendingOffset/2)/csmBlendingOffset);
	}

	// Only needed with single shadow map that doesn't cover all the scene.
	// Since we use csm this problem doesn't arise so we don't need to perform check.
	/*// far plane black region
	if(projCoords.z > 1.0)
        shadow = 0.0;*/

    return shadow;
}  

float calculateShadowAtLayer(int layer, vec3 normal, vec3 lightDir) { 
	float shadow = 0.0f;
	vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(FragPosWorldSpace, 1.0);
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// check whether current frag pos is in shadow

	// Cullface back with bias creates peter panning (shadow offset from object).
	// Cullface front with bias doesn't shadow close surfaces behind the object.

	// Default bias to modify from.
	// float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	float bias = max(shadowBiasMultiplier * (1.0 - dot(normal, lightDir)), shadowBiasMinimum);
	bias *= 1 / (cascadePlaneDistances[layer+1] * 0.5f);

	// Pcf.
	vec2 texelSize = 1.0 / vec2(textureSize(shadowSampler, 0));
	if(usePcf && usePoissonPcf) {
		shadow = calculatePoissonPcfShadow(projCoords, texelSize, layer, currentDepth, bias);
	} else if(usePcf) {
		shadow = calculatePcfShadow(projCoords, texelSize, layer, currentDepth, bias);
	} else {
		// Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords).
		float closestDepth = texture(shadowSampler, vec3(projCoords.xy, layer)).r; 
		shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;  
	}
	return shadow;
}

// At higher distances and with higher poisson radius everything starts to self shadow.
// This happens because the depth of near or adjacent pixels in distant cascades can change by a lot.
// When this happens the bias isn't enough and we get shadow acne.
// Either decrease poisson radius or find a way to change bias to be better.
float calculatePoissonPcfShadow(vec3 projCoords, vec2 texelSize, int layer, float currentDepth, float bias) {
	float shadow = 0.0f;
	float randomValue = random(gl_FragCoord.xy);
	for(int i = 0; i < pcfSamplesNumber; ++i) {
		float pcfDepth = texture(shadowSampler, vec3(projCoords.xy + rotatePcfPoint(pcfSamples[i], randomValue) * poissonPcfDiameter * texelSize * pcfMultipliers[layer], layer)).r;
		shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0; 
	}
	shadow /= pcfSamplesNumber;
	return shadow;
}

float calculatePcfShadow(vec3 projCoords, vec2 texelSize, int layer, float currentDepth, float bias) {
	float shadow = 0.0f;
	float kernel[3][3] = {
		{1.0f, 2.0f, 1.0f},
		{2.0f, 4.0f, 2.0f},
		{1.0f, 2.0f, 1.0f}
	};
	float sum = 0.0f;
	for(int x = 0; x < 3; ++x) {
		for(int y = 0; y < 3; ++y) {
			sum+=kernel[x][y];
		}
	}
	for(int x = 0; x < 3; ++x) {
		for(int y = 0; y < 3; ++y) {
			kernel[x][y]=kernel[x][y]/sum;
		}
	}
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowSampler, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r; 
			shadow += currentDepth - bias > pcfDepth ?  kernel[x+1][y+1] : 0.0;     
		}    
	}
	return shadow;
}

float random(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 rotatePcfPoint(vec2 p, float randomValue) {
	float angle = randomValue * 2 * PI;
	float s = sin(angle);
	float c = cos(angle);
	float xnew = p.x * c - p.y * s;
	float ynew = p.x * s + p.y * c;
	return vec2(xnew, ynew);
}