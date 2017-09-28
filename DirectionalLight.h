#pragma once

#include "Component.h"
#include "mat.h"
#include "texture.h"

class DirectionalLight : public Component
{
private:
	float strength;
	Color color;

public:
	DirectionalLight(float s, Color c) : strength(s), color(c) {}

	float GetStrength() { return strength; }

	Color GetColor() { return color; }
};
