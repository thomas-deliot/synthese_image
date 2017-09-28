#include "Camera.h"
#include "GameObject.h"

void Camera::SetParameters(const float width, const float height, const float fov, const float nearZ, const float farZ)
{
	projectionMatrix = Perspective(fov, width / height, nearZ, farZ);
	//projectionMatrix = GetOrthographicMatrix(150.0f, 150.0f, 0.1f, 1000.0f);
}

Transform Camera::GetProjectionMatrix()
{
	return projectionMatrix;
}

Transform Camera::GetViewMatrix()
{
	return gameObject->GetObjectToWorldMatrix().inverse();
}

Transform Camera::GetOrthographicMatrix(const float width, const float height, const float znear, const float zfar)
{
	float w = width / 2.f;
	float h = height / 2.f;

	return Transform(
		1 / w, 0, 0, 0,
		0, 1 / h, 0, 0,
		0, 0, -2 / (zfar - znear), -(zfar + znear) / (zfar - znear),
		0, 0, 0, 1);
}