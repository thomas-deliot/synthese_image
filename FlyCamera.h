#pragma once

#include "Component.h"
#include "window.h"
#include "mat.h"

class FlyCamera : public Component
{
private:
	float speed = 0.5f;

public:
	/*------------- -------------*/
	void Start()
	{
		gameObject->RotateAround(gameObject->GetForwardVector(), 180.0f);
	}

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

		// Keyboard Z rotation
		float rotationZ = 0.0f;
		if (key_state(SDLK_a))
			rotationZ = -2.0f;
		if (key_state(SDLK_e))
			rotationZ = 2.0f;

		// Mouse aiming
		int mx, my;
		unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);

		// Update game object
		Vector position = gameObject->GetPosition();
		position = position + translationX * speed * gameObject->GetRightVector()
			+ translationZ * speed * gameObject->GetForwardVector();
		gameObject->SetPosition(position);

		gameObject->RotateAround(Vector(0, 1, 0), mx * 0.5f);
		gameObject->RotateAround(gameObject->GetRightVector(), my * 0.5f);
		gameObject->RotateAround(gameObject->GetForwardVector(), rotationZ * 0.5f);
	}
	/*------------- -------------*/
};
