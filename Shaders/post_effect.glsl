
#version 330

#ifdef VERTEX_SHADER
uniform mat4 mvpMatrix;

attribute  vec2 position;

out vec2 vtexcoord;

void main( )
{
	gl_Position = vec4(position, 0, 1);
	vtexcoord = (position + 1.0) / 2.0;
}
#endif




#ifdef FRAGMENT_SHADER
uniform sampler2D colorBuffer;

in vec2 vtexcoord;

out vec4 pixelColor;

void main()
{
	pixelColor = texture(colorBuffer, vtexcoord.xy) * 0.5f;
}
#endif
