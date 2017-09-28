#pragma once

#include "Component.h"
#include "window.h"
#include "mat.h"

class FlyCamera : public Component
{
private:

public:
	/*------------- -------------*/

	/*!
	*  \brief Fonction appelée à chaque frame de l'application. Fais suivre la rotation/position de la caméra en fonction du mouvement de
	*	la souris et des deplacements au clavier.
	*/
	void Update(float dt)
	{
		// Keyboard X translation
		float translationX = 0.0f;
		if (key_state(SDLK_q))
			translationX = -2.0f;
		if (key_state(SDLK_d))
			translationX = 2.0f;

		// Keyboard Z translation
		float translationZ = 0.0f;
		if (key_state(SDLK_z))
			translationZ = -2.0f;
		if (key_state(SDLK_s))
			translationZ = 2.0f;

		// Mouse aiming
		int mx, my;
		unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);

		// Update game object
		Vector position = gameObject->GetPosition();
		position = position + translationX * gameObject->GetRightVector()
			+ translationZ * gameObject->GetForwardVector();
		gameObject->SetPosition(position);

		gameObject->RotateAround(Vector(0, 1, 0), mx);
		gameObject->RotateAround(gameObject->GetRightVector(), my);
	}
	/*------------- -------------*/
};
