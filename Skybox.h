#pragma once

#include "Component.h"
#include "Camera.h"
#include "mat.h"
#include "program.h"

using namespace std;

class Camera;

/*!
*  \brief Classe gérant un maillage et sa texture associés à un GameObject.
*/
class Skybox : public Component
{
private:
	GLuint vbo;
	GLuint vao;
	GLuint texCube;
	GLuint skyboxProgram;

public:
	void Start()
	{
		float points[] = {
			-10.0f,  10.0f, -10.0f,
			-10.0f, -10.0f, -10.0f,
			10.0f, -10.0f, -10.0f,
			10.0f, -10.0f, -10.0f,
			10.0f,  10.0f, -10.0f,
			-10.0f,  10.0f, -10.0f,

			-10.0f, -10.0f,  10.0f,
			-10.0f, -10.0f, -10.0f,
			-10.0f,  10.0f, -10.0f,
			-10.0f,  10.0f, -10.0f,
			-10.0f,  10.0f,  10.0f,
			-10.0f, -10.0f,  10.0f,

			10.0f, -10.0f, -10.0f,
			10.0f, -10.0f,  10.0f,
			10.0f,  10.0f,  10.0f,
			10.0f,  10.0f,  10.0f,
			10.0f,  10.0f, -10.0f,
			10.0f, -10.0f, -10.0f,

			-10.0f, -10.0f,  10.0f,
			-10.0f,  10.0f,  10.0f,
			10.0f,  10.0f,  10.0f,
			10.0f,  10.0f,  10.0f,
			10.0f, -10.0f,  10.0f,
			-10.0f, -10.0f,  10.0f,

			-10.0f,  10.0f, -10.0f,
			10.0f,  10.0f, -10.0f,
			10.0f,  10.0f,  10.0f,
			10.0f,  10.0f,  10.0f,
			-10.0f,  10.0f,  10.0f,
			-10.0f,  10.0f, -10.0f,

			-10.0f, -10.0f, -10.0f,
			-10.0f, -10.0f,  10.0f,
			10.0f, -10.0f, -10.0f,
			10.0f, -10.0f, -10.0f,
			-10.0f, -10.0f,  10.0f,
			10.0f, -10.0f,  10.0f
		};
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 3 * 36 * sizeof(float), &points, GL_STATIC_DRAW);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		skyboxProgram = read_program("m2tp/Shaders/skybox.glsl");
	}

	void Draw(Camera* target)
	{
		glDepthMask(GL_FALSE);
		glUseProgram(skyboxProgram);

		Transform p = target->GetProjectionMatrix();
		Transform v = target->GetViewMatrix();
		v.m[0][3] = 0.0f;
		v.m[1][3] = 0.0f;
		v.m[2][3] = 0.0f;
		glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_TRUE, p.buffer());
		glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_TRUE, v.buffer());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texCube);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthMask(GL_TRUE);
	}

	void CreateCubeMap(
		const char* front,
		const char* back,
		const char* top,
		const char* bottom,
		const char* left,
		const char* right);

	bool load_cube_map_side(GLuint texture, GLenum side_target, const char* file_name);
};
