#pragma once

#include "Component.h"
#include "mat.h"
#include "texture.h"

class PointLight : public Component
{
private:
	float range;
	float strength;
	Color color;

public:
	PointLight(float r, float s, Color c) : range(r), strength(s), color(c) {}

	float GetRange() { return range; }

	float GetStrength() { return strength; }

	Color GetColor() { return color; }
};
