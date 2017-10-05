#include "Camera.h"
#include "GameObject.h"
#include <vector>

Transform Camera::GetViewMatrix()
{
	return gameObject->GetObjectToWorldMatrix().inverse();
}

vector<Vector> Camera::GetFrustumNearCorners()
{
	float aspect = frameWidth / frameHeight;
	float fovWHalf = fov * 0.5f;
	float deg2rad = 0.01745329251f;

	Vector toRight = gameObject->GetRightVector() * nearZ * tan(fovWHalf * deg2rad) * aspect;
	Vector toTop = gameObject->GetUpVector() * nearZ * tan(fovWHalf * deg2rad);

	vector<Vector> res;
	res.push_back(gameObject->GetForwardVector() * nearZ - toRight + toTop); // Top Left
	res.push_back(gameObject->GetForwardVector() * nearZ + toRight + toTop); // Top Right
	res.push_back(gameObject->GetForwardVector() * nearZ + toRight - toTop); // Bottom Right
	res.push_back(gameObject->GetForwardVector() * nearZ - toRight - toTop); // Bottom Left

	return res;
}