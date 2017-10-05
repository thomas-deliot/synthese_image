#include "Camera.h"
#include "GameObject.h"

Transform Camera::GetViewMatrix()
{
	return gameObject->GetObjectToWorldMatrix().inverse();
}
