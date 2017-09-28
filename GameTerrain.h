#pragma once

#include "Component.h"
#include "window.h"
#include "mat.h"

#include "CImg.h"
#include "GameObject.h"
#include "MeshRenderer.h"

using namespace cimg_library;

/*!
*  \brief Classe gérant un terrain généré procéduralement à partir d'une carte d'élevation.
*/
class GameTerrain : public Component
{
private:
	MeshRenderer* renderer;
	float terrainSize = 50.0f;
	float whiteAltitude = 10.0f;
	int verticesPerLine = 100.0f;
	string heightmap;
	CImg<float> image;

public:
	/*------------- -------------*/

	/*!
	*  \brief Constructeur pour préciser tous les paramètres de terrain en une fois.
	*  \param h le chemin vers la carte d'élevation.
	*  \param wA l'altitude souhaitée des zones blanches de la carte d'élevation.
	*  \param tS la taille du terrain.
	*  \param vP le nombre de vertices par ligne souhaité pour le mesh du terrain (nombre total = vP * vP).
	*/
	GameTerrain(string h, float wA, float tS, int vP)
	{
		whiteAltitude = wA;
		terrainSize = tS;
		verticesPerLine = vP;
		heightmap = h;
	}

	/*!
	*  \brief Fonction appellée au lancement de l'application. Initialise le terrain avec
	*	les paramètres donnés au constructeur.
	*/
	void OnStart()
	{
		renderer = this->gameObject->GetComponent<MeshRenderer>();
		image = CImg<float>(heightmap.c_str());
		GenerateTerrainMesh();
	}
	/*------------- -------------*/

	/*!
	*  \brief Fonction d'interpolation linéaire entre deux valeurs.
	*  \param a la valeur de début.
	*  \param b la valeur de fin.
	*  \param f la quantité de la valeur de fin à retourner (entre 0.0f et 1.0f).
	*  \return la valeur interpolée.
	*/
	float Lerp(float a, float b, float f)
	{
		return (a * (1.0f - f)) + (b * f);
	}

	/*!
	*  \brief Génère le maillage du terrain à partir de la carte d'élévation et
	*	des paramètres fournis dans le constructeur. Assigne le mesh au MeshRenderer
	*	sur le même GameObject (doit être présent).
	*/
	void GenerateTerrainMesh()
	{
		Mesh meshHF(GL_TRIANGLES);

		float texelX = 1.0f / ((float)image.width());
		float texelY = 1.0f / ((float)image.height());

		for (int i = 0; i < verticesPerLine; i++)
		{
			for (int j = 0; j < verticesPerLine; j++)
			{
				float u = j / ((float)verticesPerLine - 1);
				float v = i / ((float)verticesPerLine - 1);

				int anchorX = u * (image.width() - 1);
				int anchorY = v * (image.height() - 1);

				if (anchorX == image.width() - 1)
					anchorX--;

				if (anchorY == image.height() - 1)
					anchorY--;

				float a = image(anchorX, anchorY, 0, 0) / 255.0f;
				float b = image(anchorX, anchorY + 1, 0, 0) / 255.0f;
				float c = image(anchorX + 1, anchorY + 1, 0, 0) / 255.0f;
				float d = image(anchorX + 1, anchorY, 0, 0) / 255.0f;


				float anchorU = anchorX * texelX;
				float anchorV = anchorY * texelY;

				float localU = (u - anchorU) / texelX;
				float localV = (v - anchorV) / texelY;

				float abu = Lerp(a, b, localU);
				float dcu = Lerp(d, c, localU);

				float height = Lerp(dcu, abu, localV) * whiteAltitude;
				meshHF.texcoord(u, 1 - v);
				//meshHF.normal(0.0f, 1.0f, 0.0f);
				meshHF.vertex(u * terrainSize, height, v * terrainSize);
			}
		}

		// Set indexes
		int nbTris = 2 * ((verticesPerLine - 1) * (verticesPerLine - 1));
		int c = 0;
		int vertexArrayLength = verticesPerLine * verticesPerLine;
		while (c < vertexArrayLength - verticesPerLine - 1)
		{
			if (c == 0 || (((c + 1) % verticesPerLine != 0) && c <= vertexArrayLength - verticesPerLine))
			{
				// First triangle of this quad
				meshHF.triangle(c, c + verticesPerLine, c + verticesPerLine + 1);

				// Second triangle of this quad
				meshHF.triangle(c + verticesPerLine + 1, c + 1, c);
			}
			c++;
		}
		renderer->SetMesh(meshHF);
	}

	/*!
	*  \brief Récupère la taille de ce terrain (longueur d'un côté du carré)
	*  \return la taille de ce terrain (longueur d'un côté du carré).
	*/
	float GetTerrainSize()
	{
		return terrainSize;
	}

	/*!
	*  \brief Récupère l'altitude dans le repère Monde du terrain aux coordonnées X,Z monde spécifiées.
	*  \param x la coordonnée X monde.
	*  \param z la coordonnée Z monde.
	*  \return la coordonnée Y monde.
	*/
	float GetWorldAltitudeAt(float x, float z)
	{
		Vector position = this->gameObject->GetPosition();
		float u = (x - position.x) / terrainSize;
		float v = (z - position.z) / terrainSize;

		if (x != x || z != z || u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
			return -9999999;

		float texelX = 1.0f / ((float)image.width());
		float texelY = 1.0f / ((float)image.height());

		int anchorX = u * (image.width() - 1);
		int anchorY = v * (image.height() - 1);

		if (anchorX == image.width() - 1)
			anchorX--;

		if (anchorY == image.height() - 1)
			anchorY--;

		float a = image(anchorX, anchorY, 0, 0) / 255.0f;
		float b = image(anchorX, anchorY + 1, 0, 0) / 255.0f;
		float c = image(anchorX + 1, anchorY + 1, 0, 0) / 255.0f;
		float d = image(anchorX + 1, anchorY, 0, 0) / 255.0f;


		float anchorU = anchorX * texelX;
		float anchorV = anchorY * texelY;

		float localU = (u - anchorU) / texelX;
		float localV = (v - anchorV) / texelY;

		float abu = Lerp(a, b, localU);
		float dcu = Lerp(d, c, localU);

		float height = Lerp(dcu, abu, localV) * whiteAltitude;
		return height + position.y;
	}
};
