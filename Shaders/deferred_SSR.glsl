
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

// SSR parameters
const float maxSteps = 256;
const float binarySearchIterations = 4;
const float jitterAmount = 1.0;
const float maxDistance = 10000.0;
const float stride = 8.0;
const float zThickness = 1.5;
const float strideZCutoff = 100.0;
const float screenEdgeFadeStart = 0.75;
const float eyeFadeStart = 0.5;
const float eyeFadeEnd = 1.0;

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
bool FindSSRHit(vec3 csOrig, vec3 csDir, float jitter, out vec2 hitPixel, out vec3 hitPoint, out float iterations);
float ComputeBlendFactorForIntersection(float iterationCount, vec2 hitPixel, vec3 hitPoint, vec3 vsRayOrigin, vec3 vsRayDirection);
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
	vec2 hitPixel = vec2(0, 0);
	vec3 hitPoint = vec3(0, 0, 0);
	vec2 uv2 = vtexcoord * renderSize;
	float jitter = mod((uv2.x + uv2.y) * 0.25, 1.0);
	float iterations = 0;
	bool hit = FindSSRHit(vsPos, vsReflect, jitter * jitterAmount, hitPixel, hitPoint, iterations);

	// Sample reflection in previous frame with temporal reprojection
	vec4 prevHit = invView * vec4(hitPoint.xyz, 1);
	prevHit = prevView * prevHit;
	prevHit = prevProj * prevHit;
	prevHit.xyz /= prevHit.w;
	
	// Blend between reprojected SSR sample and skybox
	float reflBlend = ComputeBlendFactorForIntersection(iterations, hitPixel, hitPoint, vsPos, vsReflect);
	if(hit == false)
		reflBlend = 0.0;
	vec4 ambientReflected = mix(textureLod(skybox, worldReflect, roughness * maxRoughnessMipMap),
		textureLod(prevColorBuffer, prevHit.xy * 0.5 + 0.5, roughness * maxRoughnessMipMap), reflBlend);
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
 
bool FindSSRHit(vec3 csOrig, vec3 csDir, float jitter,
	out vec2 hitPixel, out vec3 hitPoint, out float iterations) 
{
	// Clip to the near plane
	float rayLength = ((csOrig.z + csDir.z * maxDistance) > -nearZ) ?
		(-nearZ - csOrig.z) / csDir.z : maxDistance;
	vec3 csEndPoint = csOrig + csDir * rayLength;
	//if(csEndPoint.z > csOrig.z)
	//	return false;

	// Project into homogeneous clip space
	vec4 H0 = projToPixel * vec4(csOrig, 1.0);
	vec4 H1 = projToPixel * vec4(csEndPoint, 1.0);
	float k0 = 1.0 / H0.w, k1 = 1.0 / H1.w;

	// The interpolated homogeneous version of the camera-space points  
	vec3 Q0 = csOrig * k0, Q1 = csEndPoint * k1;

	// Screen-space endpoints
	vec2 P0 = H0.xy * k0, P1 = H1.xy * k1;

	// If the line is degenerate, make it cover at least one pixel
	// to avoid handling zero-pixel extent as a special case later
	P1 += vec2((DistanceSquared(P0, P1) < 0.0001) ? 0.01 : 0.0);
	vec2 delta = P1 - P0;

	// Permute so that the primary iteration is in x to collapse
	// all quadrant-specific DDA cases later
	bool permute = false;
	if (abs(delta.x) < abs(delta.y)) 
	{
		// This is a more-vertical line
		permute = true;
		delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx; 
	}
	
	float stepDir = sign(delta.x);
	float invdx = stepDir / delta.x;
	
	// Track the derivatives of Q and k
	vec3  dQ = (Q1 - Q0) * invdx;
	float dk = (k1 - k0) * invdx;
	vec2  dP = vec2(stepDir, delta.y * invdx);

	float strideScaler = 1.0 - min( 1.0, -csOrig.z / strideZCutoff);
	float pixelStride = 1.0 + strideScaler * stride;

	// Scale derivatives by the desired pixel stride and then
	// offset the starting values by the jitter fraction
	dP *= pixelStride; dQ *= pixelStride; dk *= pixelStride;
	P0 += dP * jitter; Q0 += dQ * jitter; k0 += dk * jitter;

	float end = P1.x * stepDir;
	float i, zA = csOrig.z, zB = csOrig.z;
	vec4 pqk = vec4(P0, Q0.z, k0);
	vec4 dPQK = vec4(dP, dQ.z, dk);
	bool intersect = false;
	for (i = 0; i < maxSteps && intersect == false && pqk.x * stepDir <= end; i++) 
	{
		pqk += dPQK;

		zA = zB;
		zB = (dPQK.z * 0.5 + pqk.z) / (dPQK.w * 0.5 + pqk.w);
 
		hitPixel = permute ? pqk.yx : pqk.xy;
		hitPixel = hitPixel / renderSize;
		float currentZ = Linear01Depth(texture(depthBuffer, hitPixel).x) * -farZ;
		intersect = zA >= currentZ - zThickness && zB <= currentZ;
	}
	
	// Binary search refinement
	float addDQ = 0.0;
	if(pixelStride > 1.0 && intersect)
	{
		pqk -= dPQK;
		dPQK /= pixelStride;
		float originalStride = pixelStride * 0.5;
	    float stride = originalStride;	        		
	    zA = pqk.z / pqk.w;
	    zB = zA;	        		
	    for(float j = 0; j < binarySearchIterations; j++)
		{
			pqk += dPQK * stride;
			addDQ += stride;
				    	
			zA = zB;
			zB = (dPQK.z * 0.5 + pqk.z) / (dPQK.w * 0.5 + pqk.w);
				    	
			hitPixel = permute ? pqk.yx : pqk.xy;				
			hitPixel = hitPixel / renderSize;
			float currentZ = Linear01Depth(texture(depthBuffer, hitPixel).x) * -farZ;
			bool intersect2 = zA >= currentZ - zThickness && zB <= currentZ;   
			
			originalStride *= 0.5;
			stride = intersect2 ? -originalStride : originalStride;
		}
	}

	// Advance Q based on the number of steps
	Q0.xy += dQ.xy * (i - 1) + (dQ.xy / pixelStride) * addDQ;
	Q0.z = pqk.z;
	hitPoint = Q0 / pqk.w;
	iterations = i;
	return intersect;
}

float ComputeBlendFactorForIntersection(
		float iterationCount, 
		vec2 hitPixel,
		vec3 hitPoint,
		vec3 vsRayOrigin,
		vec3 vsRayDirection)
{
	float alpha = 1.0f;

	// Fade ray hits that approach the maximum iterations
	alpha *= 1.0 - pow(iterationCount / maxSteps, 8.0);

	// Fade ray hits that approach the screen edge
	float screenFade = screenEdgeFadeStart;
	vec2 hitPixelNDC = (hitPixel * 2.0 - 1.0);
	float maxDimension = min( 1.0, max( abs( hitPixelNDC.x), abs( hitPixelNDC.y)));
	alpha *= 1.0 - (max( 0.0, maxDimension - screenFade) / (1.0 - screenFade));

	// Fade ray hits base on how much they face the camera
	float eyeDirection = clamp(vsRayDirection.z, eyeFadeStart, eyeFadeEnd);
	alpha *= 1.0 - ((eyeDirection - eyeFadeStart) / (eyeFadeEnd - eyeFadeStart));

	// Fade ray hits based on distance from ray origin
	//alpha *= 1.0 - clamp(distance(vsRayOrigin, hitPoint) / maxDistance, 0.0, 1.0);

	return alpha;
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
