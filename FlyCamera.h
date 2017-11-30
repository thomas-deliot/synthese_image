#pragma once

#include "Component.h"
#include "window.h"
#include "mat.h"

class FlyCamera : public Component
{
private:
	float speed = 0.5f;
	bool firstFrame = true;

public:
	/*------------- -------------*/
	void Start()
	{

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
		if (firstFrame == true)
		{
			mx = 0;
			my = 0;
			firstFrame = false;
		}
		float mx2 = mx * 0.5f;
		float my2 = my * 0.5f;
		rotationZ *= 0.5f;

		// Update game object
		Vector position = gameObject->GetPosition();
		position = position + translationX * speed * gameObject->GetRightVector()
			+ translationZ * speed * gameObject->GetForwardVector();
		gameObject->SetPosition(position);

		TQuaternion<float, Vector> prevRot = gameObject->GetRotation();
		gameObject->RotateAround(Vector(0, 1, 0), mx2);
		gameObject->RotateAround(gameObject->GetRightVector(), my2);
		gameObject->RotateAround(gameObject->GetForwardVector(), rotationZ);

		// Clamp vertical look
		Vector forward = gameObject->GetForwardVector();
		if (forward.y > 0.99f || forward.y < -0.99f)
			gameObject->SetRotation(prevRot);
	}
	/*------------- -------------*/
};
