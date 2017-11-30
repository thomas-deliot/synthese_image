
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
uniform float nearZ;
uniform float farZ;
uniform mat4 invProj;
uniform mat4 invView;

uniform vec3 camPos;
uniform vec4 ambientLight;
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform float lightStrength;

in vec2 vtexcoord;

out vec4 finalColor;

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

	// Diffuse term (Lambert)
	float diffTerm = max(0.0, dot(-lightDir, worldNormal));

	// Specular term (Blinn Phong)
	float specular = 0;
	if(diffTerm > 0)
	{
		vec4 clipPos = vec4(vtexcoord.xy * 2.0 - 1.0, 0.5, 1);
		vec4 temp = invProj * clipPos;
		temp /= temp.w;
		vec3 worldPos = mat3(invView) * temp.xyz;
	
		vec3 viewDir = normalize(camPos - worldPos);
		vec3 halfDir = normalize(-lightDir + viewDir);
		float specAngle = max(dot(halfDir, worldNormal), 0.0);
		specular = pow(specAngle, shininess);
	}

	// Final color
	finalColor = ambientLight
		+ diffTerm * diffuseColor * (lightColor * lightStrength)
		+ specular * (lightColor * lightStrength);
	//finalColor = vec4(worldNormal.xyz, 1);
}
#endif
