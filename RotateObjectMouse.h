#pragma once

#include "Component.h"
#include "window.h"
#include "mat.h"

using namespace std;

class RotateObjectMouse : public Component
{
private:
	float sensitivity = 1.0f;

public:
	/*------------- -------------*/
	void Update(float dt)
	{
		// Mouse rotation
		int mx, my;
		unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);

		// Update game object
		gameObject->RotateAround(Vector(0, 1, 0), -mx * sensitivity);
		gameObject->RotateAround(Vector(1, 0, 0), -my * sensitivity);
	}
	/*------------- -------------*/
};
