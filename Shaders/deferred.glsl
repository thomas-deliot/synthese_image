
#version 330

#ifdef VERTEX_SHADER
const vec2 quadVertices[4] = vec2[4]( vec2( -1.0, -1.0), vec2( 1.0, -1.0), vec2( -1.0, 1.0), vec2( 1.0, 1.0));

out vec2 vtexcoord;

void main( )
{
	gl_Position = vec4(quadVertices[gl_VertexID], 0.0, 1.0);
	vtexcoord = (quadVertices[gl_VertexID] + 1.0) / 2.0;
}
#endif




#ifdef FRAGMENT_SHADER
uniform sampler2D colorBuffer;
uniform sampler2D normalBuffer;
uniform sampler2D depthBuffer;
uniform sampler2D prevColorBuffer;
uniform samplerCube skybox;

uniform vec2 renderSize;
uniform mat4 viewMatrix;
uniform mat4 projToPixel;
uniform mat4 invProj;
uniform mat4 prevProj;
uniform mat4 invView;
uniform mat4 prevView;
uniform float nearZ;
uniform float farZ;
const float PI = 3.14159265359f;

// Lighting parameters
uniform vec3 camPos;
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform float lightStrength;
const float maxRoughnessMipMap = 7.0;

in vec2 vtexcoord;
out vec4 finalColor;

float rand(vec2 co);
float GetRandomNumberBetween(vec2 co, float minimum, float maximum);
float DistanceSquared(vec2 a, vec2 b);
float Linear01Depth(float z);
float LinearEyeDepth(float z);
vec3 colorForDirectionalLight(vec3 vsPos, vec3 vsNormal, vec3 vsLightDir, vec3 incomingColor, vec3 surfaceColor, float metalness, float roughness);
vec3 colorForIBL(vec3 vsPos, vec3 vsNormal, vec3 incomingAmbient, vec3 incomingReflected, vec3 surfaceColor, float metalness, float roughness);
vec3 colorLinear(vec3 colorVector);
float saturate(float f);
vec2 getSphericalCoord(vec3 normalCoord);
float Fd90(float NoL, float roughness);
float KDisneyTerm(float NoL, float NoV, float roughness);
vec3 computeFresnelSchlick(float NdotV, vec3 F0);
vec3 computeFresnelSchlickRoughness(float NdotV, vec3 F0, float roughness);
float computeDistributionGGX(vec3 N, vec3 H, float roughness);
float computeGeometryAttenuationGGXSmith(float NdotL, float NdotV, float roughness);


void main()
{
	// Sample GBuffer information
	vec4 temp = texture(colorBuffer, vtexcoord.xy);
	vec4 temp2 = texture(normalBuffer, vtexcoord.xy);
	vec3 albedo = temp.rgb;
	vec3 worldNormal = temp2.rgb;
	vec3 vsNormal = mat3(viewMatrix) * worldNormal;
	vec3 vsLightDir = mat3(viewMatrix) * lightDir;
	float roughness = temp.a;
	float metalness = temp2.a;
	float z = texture(depthBuffer, vtexcoord).x;
	if(z >= 0.9999f)
	{
		finalColor = vec4(albedo.rgb, 1);
		return;
	}

	// Screen Space Reflection Test
	vec4 clipSpacePosition = vec4(vtexcoord.xy * 2.0 - 1.0, z * 2.0 - 1.0, 1);
	vec4 viewSpacePosition = invProj * clipSpacePosition;
	viewSpacePosition /= viewSpacePosition.w;
	vec3 vsPos = viewSpacePosition.xyz;
	vec3 vsRayDir = normalize(vsPos);
	vec3 vsReflect = reflect(vsRayDir, vsNormal);
	vec3 worldReflect = (invView * vec4(vsReflect.xyz, 0)).xyz;

	vec4 ambientReflected = textureLod(skybox, worldReflect, roughness * maxRoughnessMipMap);
	vec4 ambientDiffuse = texture(skybox, worldNormal);
		
	// Directional Light + Reflection light
	vec3 color = vec3(0, 0, 0);
	color += colorForDirectionalLight(vsPos, vsNormal, vsLightDir, lightColor.rgb * lightStrength, albedo, metalness, roughness);
	color += colorForIBL(vsPos, vsNormal, ambientDiffuse.rgb, ambientReflected.rgb, albedo, metalness, roughness);
	
	finalColor = vec4(color.rgb, 1);
}





// ---------- Functions ------------
float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float GetRandomNumberBetween(vec2 co, float minimum, float maximum)
{
	float f0 = rand(co);
	return minimum + f0 * (maximum - minimum);
}

float DistanceSquared(vec2 a, vec2 b) 
{ 
	a -= b; 
	return dot(a, a); 
}

float Linear01Depth(float z)
{
	float temp = farZ / nearZ;
	return 1.0 / ((1.0 - temp) * z + temp);
}

float LinearEyeDepth(float z)
{
	float temp = farZ / nearZ;
	return 1.0 / ((1.0 - temp) / farZ * z + temp / farZ);
}

vec3 colorForDirectionalLight(vec3 vsPos, vec3 vsNormal, vec3 vsLightDir, vec3 incomingColor, vec3 surfaceColor, float metalness, float roughness)
{
	vec3 V = normalize(-vsPos);
	vec3 N = normalize(vsNormal);
	vec3 R = reflect(-V, N);
	float NdotV = max(dot(N, V), 0.0001);

	// Fresnel (Schlick) computation (F term)
	vec3 F0 = mix(vec3(0.04, 0.04, 0.04), surfaceColor, metalness);
	vec3 F = computeFresnelSchlick(NdotV, F0);

	// Energy conservation
	vec3 kS = F;
	vec3 kD = vec3(1.0, 1.0, 1.0) - kS;
	kD *= 1.0 - metalness;
	
	vec3 L = normalize(vsLightDir);
    vec3 H = normalize(L + V);

    // Light source dependent BRDF term(s)
    float NdotL = saturate(dot(N, L));

    // Diffuse component computation
    vec3 diffuse = surfaceColor / PI;

    // Disney diffuse term
    float kDisney = KDisneyTerm(NdotL, NdotV, roughness);

    // Distribution (GGX) computation (D term)
    float D = computeDistributionGGX(N, H, roughness);

    // Geometry attenuation (GGX-Smith) computation (G term)
    float G = computeGeometryAttenuationGGXSmith(NdotL, NdotV, roughness);

    // Specular component computation
    vec3 specular = (F * D * G) / (4.0f * NdotL * NdotV + 0.0001);

	return (diffuse * kD + specular) * incomingColor * NdotL;
}

vec3 colorForIBL(vec3 vsPos, vec3 vsNormal, vec3 incomingAmbient, vec3 incomingReflected, vec3 surfaceColor, float metalness, float roughness)
{
	vec3 V = normalize(-vsPos);
	vec3 N = normalize(vsNormal);
	vec3 R = reflect(-V, N);
	float NdotV = max(dot(N, V), 0.0001);

	// Fresnel (Schlick) computation (F term)
	vec3 F0 = mix(vec3(0.04, 0.04, 0.04), surfaceColor, metalness);
	vec3 F = computeFresnelSchlickRoughness(NdotV, F0, roughness);
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metalness;
	
	// Diffuse irradience computation
	vec3 diffuseIrradiance = incomingAmbient;
	diffuseIrradiance *= surfaceColor;
	
	// Specular radiance computation
	vec3 specularRadiance = incomingReflected;
	specularRadiance *= F;
	
	vec3 ambientIBL = (diffuseIrradiance * kD) + specularRadiance;
	return ambientIBL;
}

vec3 colorLinear(vec3 colorVector)
{
    vec3 linearColor = pow(colorVector.rgb, vec3(2.2f));
    return linearColor;
}

float saturate(float f)
{
    return clamp(f, 0.0f, 1.0f);
}

vec2 getSphericalCoord(vec3 normalCoord)
{
    float phi = acos(-normalCoord.y);
    float theta = atan(1.0f * normalCoord.x, -normalCoord.z) + PI;
    return vec2(theta / (2.0f * PI), phi / PI);
}

float Fd90(float NoL, float roughness)
{
    return (2.0f * NoL * roughness) + 0.4f;
}

float KDisneyTerm(float NoL, float NoV, float roughness)
{
    return (1.0f + Fd90(NoL, roughness) * pow(1.0f - NoL, 5.0f)) * (1.0f + Fd90(NoV, roughness) * pow(1.0f - NoV, 5.0f));
}

vec3 computeFresnelSchlick(float NdotV, vec3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - NdotV, 5.0f);
}

vec3 computeFresnelSchlickRoughness(float NdotV, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(1.0f - NdotV, 5.0f);
}

float computeDistributionGGX(vec3 N, vec3 H, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;

    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    return (alpha2) / (PI * (NdotH2 * (alpha2 - 1.0f) + 1.0f) * (NdotH2 * (alpha2 - 1.0f) + 1.0f));
}

float computeGeometryAttenuationGGXSmith(float NdotL, float NdotV, float roughness)
{
    float NdotL2 = NdotL * NdotL;
    float NdotV2 = NdotV * NdotV;
    float kRough2 = roughness * roughness + 0.0001f;

    float ggxL = (2.0f * NdotL) / (NdotL + sqrt(NdotL2 + kRough2 * (1.0f - NdotL2)));
    float ggxV = (2.0f * NdotV) / (NdotV + sqrt(NdotV2 + kRough2 * (1.0f - NdotV2)));

    return ggxL * ggxV;
}
#endif
