#pragma once

#include "Component.h"
#include "Camera.h"

#include "mat.h"
#include "wavefront.h"
#include "texture.h"
#include "program.h"
#include "uniforms.h"

using namespace std;

/*!
*  \brief Classe gérant un maillage et sa texture associés à un GameObject.
*/
class MeshRenderer : public Component
{
private:
	Mesh mesh;
	GLuint texture;
	GLuint shaderProgram;
	Color color = Color(1,0,0,1);

public:
	MeshRenderer(){}

	/*!
	*  \brief fonction appelée à la fermeture de l'application pour tous les composants.
	*/
	void OnDestroy()
	{
		release_program(shaderProgram);
	}

	/*------------- -------------*/
	void Draw(Camera* target)
	{
		// Setup shader program for draw
		glUseProgram(shaderProgram);
		Transform mvp = target->GetProjectionMatrix() * (target->GetViewMatrix() * gameObject->GetObjectToWorldMatrix());
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvpMatrix"), 1, GL_TRUE, mvp.buffer());
		glUniform4fv(glGetUniformLocation(shaderProgram, "color"), 1, &color.r);

		// Draw mesh
		GLuint vao = mesh.GetVAO();
		if (vao == 0)
			mesh.create_buffers();
		glBindVertexArray(mesh.GetVAO());
		glUseProgram(shaderProgram);
		glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices().size(), GL_UNSIGNED_INT, 0);
	}

	/*!
	*  \brief Récupère le maillage de ce GameObject (par référence).
	*  \return le maillage de ce GameObject (par référence).
	*/
	Mesh& GetMesh()
	{
		return mesh;
	}

	/*!
	*  \brief Assigne un maillage à ce GameObject.
	*  \param m le maillage à assigner.
	*/
	void SetMesh(Mesh m)
	{
		mesh = m;
	}

	void SetColor(Color c)
	{
		color = c;
	}

	/*!
	*  \brief Récupère la texture associée à ce GameObject.
	*  \return la texture associée à ce GameObject.
	*/
	GLuint GetTexture()
	{
		return texture;
	}

	/*!
	*  \brief Récupère le shader associée à ce GameObject.
	*  \return le shader associée à ce GameObject.
	*/
	GLuint GetShader()
	{
		return shaderProgram;
	}

	/*!
	*  \brief Assigne un maillage à ce GameObject à partir d'un fichier.
	*  \param filename le chemin du fichier contenant le maillage à charger.
	*/
	void LoadMesh(const char *filename)
	{
		mesh = read_mesh(filename);
	}

	/*!
	*  \brief Assigne une texture à ce GameObject à partir d'un fichier.
	*  \param filename le chemin du fichier contenant la texture à charger.
	*/
	void LoadTexture(const char *filename)
	{
		texture = read_texture(0, filename);
	}

	/*!
	*  \brief Assigne un shader à ce GameObject à partir d'un fichier.
	*  \param filename le chemin du fichier contenant le shader à charger.
	*/
	void LoadShader(const char *filename)
	{
		shaderProgram = read_program(filename);
	}
	/*------------- -------------*/

};
