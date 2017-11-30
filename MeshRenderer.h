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
	GLuint albedoTex;
	GLuint roughTex;
	GLuint metalTex;
	GLuint shaderProgram;
	Color color = Color(1, 1, 1, 1);
	float roughness = 0.0f;
	float metalness = 0.0f;

public:
	MeshRenderer() {}

	/*!
	*  \brief fonction appelée à la fermeture de l'application pour tous les composants.
	*/
	void OnDestroy()
	{
		release_program(shaderProgram);
		glDeleteTextures(1, &albedoTex);
		glDeleteTextures(1, &roughTex);
		glDeleteTextures(1, &metalTex);
	}

	/*------------- -------------*/
	void Draw(Camera* target)
	{
		// Setup shader program for draw
		glUseProgram(shaderProgram);
		Transform mvp = target->GetProjectionMatrix() * (target->GetViewMatrix() * gameObject->GetObjectToWorldMatrix());
		Transform trs = gameObject->GetObjectToWorldMatrix();
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvpMatrix"), 1, GL_TRUE, mvp.buffer());
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "trsMatrix"), 1, GL_TRUE, trs.buffer());
		glUniform4fv(glGetUniformLocation(shaderProgram, "color"), 1, &color.r);
		glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), roughness);
		glUniform1f(glGetUniformLocation(shaderProgram, "metalness"), metalness);

		// If use texture
		int id = glGetUniformLocation(shaderProgram, "albedoTex");
		if (id >= 0 && albedoTex >= 0)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, albedoTex);
			glUniform1i(id, 0);
		}
		id = glGetUniformLocation(shaderProgram, "roughTex");
		if (id >= 0 && roughTex >= 0)
		{
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, roughTex);
			glUniform1i(id, 1);
		}
		id = glGetUniformLocation(shaderProgram, "metalTex");
		if (id >= 0 && metalTex >= 0)
		{
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, metalTex);
			glUniform1i(id, 2);
		}

		// Draw mesh
		GLuint vao = mesh.GetVAO();
		if (vao == 0)
			mesh.create_buffers();
		glBindVertexArray(mesh.GetVAO());
		glUseProgram(shaderProgram);

		if (mesh.indices().size() > 0)
			glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices().size(), GL_UNSIGNED_INT, 0);
		else
			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mesh.positions().size());
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
		return albedoTex;
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
	void LoadMesh(const char* filename)
	{
		mesh = read_mesh(filename);
	}

	/*!
	*  \brief Assigne une texture à ce GameObject à partir d'un fichier.
	*  \param filename le chemin du fichier contenant la texture à charger.
	*/
	void LoadTexture(const char* filename, float r, float m)
	{
		albedoTex = read_texture(0, filename);
		roughness = r;
		metalness = m;
	}

	void LoadPBRTextures(const char* a, const char* r, const char* m)
	{
		albedoTex = read_texture(0, a);
		roughTex = read_texture(0, r);
		metalTex = read_texture(0, m);
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
