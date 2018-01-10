
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

const float PI = 3.14159265359f;
const float ssaoStrength = 4.0; // 0.0 is no ssao, higher is stronger
const float ssaoDist = 3.0;
const float ssaoPiDivider = 10.0; // fibonacci directions step = PI / ssaoPiDivider, higher is more samples

in vec2 vtexcoord;
out vec4 finalColor;

float rand(vec2 co);
float GetRandomNumberBetween(vec2 co, float minimum, float maximum);
float ComputeSSAOAtten(vec3 vsPos, vec3 vsNormal);


void main()
{
	// Sample GBuffer information
	vec4 temp = texture(colorBuffer, vtexcoord.xy);
	vec4 temp2 = texture(normalBuffer, vtexcoord.xy);
	vec3 worldNormal = temp2.rgb;
	vec3 vsNormal = mat3(viewMatrix) * worldNormal;
	float z = texture(depthBuffer, vtexcoord).x;
	if(z >= 0.9999f)
		return temp;

	vec4 clipSpacePosition = vec4(vtexcoord.xy * 2.0 - 1.0, z * 2.0 - 1.0, 1);
	vec4 viewSpacePosition = invProj * clipSpacePosition;
	viewSpacePosition /= viewSpacePosition.w;
	vec3 vsPos = viewSpacePosition.xyz;
	
	finalColor = vec4(temp.rgb * ComputeSSAOAtten(vsPos, vsNormal), temp.a);
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

float ComputeSSAOAtten(vec3 vsPos, vec3 vsNormal)
{
	vec3 upDir = vec3(0.0f, 1.0f, 0.0f);
	vec3 rightDir = cross(upDir, vsNormal);
	upDir = cross(vsNormal, rightDir);
	float sampleOffset = PI / ssaoPiDivider;
	float hitCount = 0.0f;
	float sampleCount = 0.0f;
	for(float anglePhi = 0.0f; anglePhi < 2.0f * PI; anglePhi += sampleOffset)
	{
		for(float angleTheta = 0.01f; angleTheta < 0.5f * PI; angleTheta += sampleOffset)
		{
			vec3 sampleTangent = vec3(sin(angleTheta) * cos(anglePhi),  sin(angleTheta) * sin(anglePhi), cos(angleTheta));
			vec3 sampleVector = sampleTangent.x * rightDir + sampleTangent.y * upDir + sampleTangent.z * vsNormal;
			
			vec2 seed = vec2(sampleVector.x + sampleTangent.z + vsNormal.y, sampleVector.z + sampleTangent.y + vsNormal.x);
			vec3 samplePos = vsPos + sampleVector * GetRandomNumberBetween(seed, 1.0, ssaoDist);
			vec4 temp = projToPixel * vec4(samplePos, 1);
			temp.xyz /= temp.w;
			
			vec2 sampleUV = temp.xy / renderSize;
			if(sampleUV.x > 1.0f || sampleUV.x < 0.0f || sampleUV.y > 1.0f || sampleUV.y < 0.0f)
				continue;
			
			float tempZ = temp.z * 0.5 + 0.5;
			float camZ = texture(depthBuffer, sampleUV).x;
			temp = invProj * vec4(sampleUV.xy * 2.0 - 1.0, camZ * 2.0 - 1.0, 1);
			temp.xyz /= temp.w;
			vec3 camPos = temp.xyz;
			
			if(tempZ > camZ && length(camPos - vsPos) < ssaoDist * 2.0)
				hitCount++;
			sampleCount++;
		}
	}
	
	float atten = hitCount / sampleCount;
	return clamp(pow(1.0 - atten, ssaoStrength), 0, 1);
}
#endif
