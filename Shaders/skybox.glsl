
#version 330

#ifdef VERTEX_SHADER
in vec3 vp;
uniform mat4 projection, view;
out vec3 texcoords;

void main() 
{
	texcoords = vp;
	gl_Position = projection * view * vec4(vp, 1.0);
}
#endif




#ifdef FRAGMENT_SHADER
in vec3 texcoords;
uniform samplerCube cube_texture;
out vec4 frag_colour;

void main() 
{
	frag_colour = texture(cube_texture, texcoords);
}
#endif
