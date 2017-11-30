
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
uniform vec2 renderSize;
uniform mat4 viewMatrix;
uniform mat4 projToPixel;
uniform mat4 invProj;
uniform mat4 invView;
uniform float nearZ;
uniform float farZ;

const float maxSteps = 256;
const float binarySearchIterations = 4;
const float maxDistance = 100.0;
const float stride = 8.0;
const float zThickness = 1.0;
const float strideZCutoff = 100.0;
const float screenEdgeFadeStart = 0.75;
const float eyeFadeStart = 0.5;
const float eyeFadeEnd = 1.0;

uniform vec3 camPos;
uniform vec4 ambientLight;
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform float lightStrength;

in vec2 vtexcoord;

out vec4 finalColor;

bool FindSSRHit(vec3 csOrig, vec3 csDir, float jitter, out vec2 hitPixel, out vec3 hitPoint, out float iterations);
float ComputeBlendFactorForIntersection(float iterationCount, vec2 hitPixel, vec3 hitPoint, vec3 vsRayOrigin, vec3 vsRayDirection);
float DistanceSquared(vec2 a, vec2 b);
float Linear01Depth(float z);
float LinearEyeDepth(float z);


void main()
{
	// Sample GBuffer information
	vec4 temp = texture(colorBuffer, vtexcoord.xy);
	vec4 diffuseColor = vec4(temp.xyz, 1);
	float shininess = temp.w * 500.0;
	vec3 worldNormal = texture(normalBuffer, vtexcoord.xy).xyz;
	float z = texture(depthBuffer, vtexcoord).x;
	if(z >= 0.9999f)
	{
		finalColor = diffuseColor;
		return;
	}

	// Calculate world pixel pos and normal
	vec4 clipSpacePosition = vec4(vtexcoord.xy * 2.0 - 1.0, z * 2.0 - 1.0, 1);
	vec4 viewSpacePosition = invProj * clipSpacePosition;
	viewSpacePosition /= viewSpacePosition.w;
	vec3 vsPos = viewSpacePosition.xyz;
	vec3 vsNormal = mat3(viewMatrix) * worldNormal;

	// Screen Space Reflection Test
	vec3 vsRayDir = normalize(vsPos);
	vec3 vsReflect = reflect(vsRayDir, vsNormal);
	//vsReflect = vec3(0,0,1);
	vec2 hitPixel = vec2(0, 0);
	vec3 hitPoint = vec3(0, 0, 0);
	vec2 uv2 = vtexcoord * renderSize;
	float jitter = mod((uv2.x + uv2.y) * 0.25, 1.0);
	float iterations = 0;
	bool hit = FindSSRHit(vsPos, vsReflect, jitter, hitPixel, hitPoint, iterations);

	// Calculate blend factor
	float reflBlend = ComputeBlendFactorForIntersection(iterations, hitPixel, hitPoint, vsPos, vsReflect);
	if(hit == false || hitPixel.x > 1.0f || hitPixel.x < 0.0f || hitPixel.y > 1.0f || hitPixel.y < 0.0f)
		reflBlend = 0;	
	vec4 hitColor = texture(colorBuffer, hitPixel.xy);
	
	
	// Final Lighting computation
	// Diffuse term (Lambert)
	float diffTerm = max(0.0, dot(-lightDir, worldNormal));
	float diffTermRefl = max(0.0, dot(vsReflect, worldNormal));

	// Specular term (Blinn Phong)
	float specular = 0;
	if(diffTerm > 0)
	{
		vec3 worldPos = mat3(invView) * vsPos;
		vec3 viewDir = normalize(camPos - worldPos);
		vec3 halfDir = normalize(-lightDir + viewDir);
		float specAngle = max(dot(halfDir, worldNormal), 0.0);
		specular = pow(specAngle, shininess);
	}
	float specularRefl = 0;
	if(diffTermRefl > 0)
	{
		vec3 worldPos = mat3(invView) * vsPos;
		vec3 viewDir = normalize(camPos - worldPos);
		vec3 halfDir = normalize(vsReflect + viewDir);
		float specAngle = max(dot(halfDir, worldNormal), 0.0);
		specularRefl = pow(specAngle, shininess);
	}

	// Final color
	finalColor = ambientLight
		+ diffTerm * diffuseColor * (lightColor * lightStrength) + specular * (lightColor * lightStrength)
		+ (diffTermRefl * diffuseColor * hitColor + specularRefl * hitColor) * reflBlend;
}





// ---------- Functions ------------
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
		/*if (zB > zA) 
		{ 
			float t = zA;
			zA = zB;
			zB = t;
		}*/
 
		hitPixel = permute ? pqk.yx : pqk.xy;
		//hitPixel.y = renderSize.y - hitPixel.y;

		hitPixel = hitPixel / renderSize;
		float currentZ = Linear01Depth(texture(depthBuffer, hitPixel).x) * -farZ;
		intersect = zA >= currentZ - zThickness && zB <= currentZ;
	}
	
	// Binary search refinement
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
				    	
			zA = zB;
			zB = (dPQK.z * 0.5 + pqk.z) / (dPQK.w * 0.5 + pqk.w);
	    	if (zB > zA) 
			{ 
				float t = zA;
				zA = zB;
				zB = t;
			}
				    	
			hitPixel = permute ? pqk.yx : pqk.xy;
			//hitPixel.y = renderSize.y - hitPixel.y;
				
			hitPixel = hitPixel / renderSize;
			float currentZ = Linear01Depth(texture(depthBuffer, hitPixel).x) * -farZ;
			bool intersect2 = zA >= currentZ - zThickness && zB <= currentZ;   
			
			originalStride *= 0.5;
			stride = intersect2 ? -originalStride : originalStride;
		}
	}

	// Advance Q based on the number of steps
	Q0.xy += dQ.xy * i;
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
	alpha *= 1.0 - (iterationCount / maxSteps);

	// Fade ray hits that approach the screen edge
	float screenFade = screenEdgeFadeStart;
	vec2 hitPixelNDC = (hitPixel * 2.0 - 1.0);
	float maxDimension = min( 1.0, max( abs( hitPixelNDC.x), abs( hitPixelNDC.y)));
	alpha *= 1.0 - (max( 0.0, maxDimension - screenFade) / (1.0 - screenFade));

	// Fade ray hits base on how much they face the camera
	float eyeDirection = clamp(vsRayDirection.z, eyeFadeStart, eyeFadeEnd);
	alpha *= 1.0 - ((eyeDirection - eyeFadeStart) / (eyeFadeEnd - eyeFadeStart));

	// Fade ray hits based on distance from ray origin
	alpha *= 1.0 - clamp(distance(vsRayOrigin, hitPoint) / maxDistance, 0.0, 1.0);

	return alpha;
}
#endif
