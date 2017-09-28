#pragma once

#include "Component.h"
#include "mat.h"

class Camera : public Component
{
private:
	//Transform projectionMatrix = Orthographic(150.0f, 150.0f, 0.1f, 1000.0f);
	Transform projectionMatrix = Perspective(60.0f, 1.0f, 0.1f, 1000.0f);

public:
	void SetParameters(const float width, const float height, const float fov, const float nearZ, const float farZ);

	/*!
	*  \brief Récupère la matrice de projection.
	*/
	Transform GetProjectionMatrix();

	/*!
	*  \brief Récupère l'inverse de la matrice du monde.
	*/
	Transform GetViewMatrix();

	/*!
	*  \brief Récupère la matrice de projection orthographique.
	*/
	Transform GetOrthographicMatrix(const float width, const float height, const float znear, const float zfar);
};
