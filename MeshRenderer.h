#pragma once

#include "Component.h"
#include "Camera.h"
#include "DirectionalLight.h"

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
	Color color = Color(1, 1, 1, 1);

public:
	MeshRenderer() {}

	/*!
	*  \brief fonction appelée à la fermeture de l'application pour tous les composants.
	*/
	void OnDestroy()
	{
		release_program(shaderProgram);
		glDeleteTextures(1, &texture);
	}

	/*------------- -------------*/
	void Draw(Camera* target, DirectionalLight* light, Color ambientLight)
	{
		// Setup shader program for draw
		glUseProgram(shaderProgram);
		Transform mvp = target->GetProjectionMatrix() * (target->GetViewMatrix() * gameObject->GetObjectToWorldMatrix());
		Transform trs = gameObject->GetObjectToWorldMatrix();
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvpMatrix"), 1, GL_TRUE, mvp.buffer());
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "trsMatrix"), 1, GL_TRUE, trs.buffer());
		glUniform4fv(glGetUniformLocation(shaderProgram, "color"), 1, &color.r);
		glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), 128.0f);

		Vector camPos = target->GetGameObject()->GetPosition();
		Vector lightDir = light->GetGameObject()->GetForwardVector();
		Color lightColor = light->GetColor();
		glUniform3fv(glGetUniformLocation(shaderProgram, "camPos"), 1, &camPos.x);
		glUniform4fv(glGetUniformLocation(shaderProgram, "ambientLight"), 1, &ambientLight.r);
		glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, &lightDir.x);
		glUniform4fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, &lightColor.r);
		glUniform1f(glGetUniformLocation(shaderProgram, "lightStrength"), light->GetStrength());

		// If use texture
		int id = glGetUniformLocation(shaderProgram, "diffuseTex");
		if (id >= 0 && texture >= 0)
		{
			int unit = 0;
			int sampler = 0;
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, texture);
			glBindSampler(unit, sampler);
			glUniform1i(id, unit);
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
