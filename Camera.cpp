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
	Vector lol = gameObject->GetForwardVector();
	res.push_back(gameObject->GetForwardVector() * nearZ - toRight + toTop); // Top Left
	res.push_back(gameObject->GetForwardVector() * nearZ + toRight + toTop); // Top Right
	res.push_back(gameObject->GetForwardVector() * nearZ + toRight - toTop); // Bottom Right
	res.push_back(gameObject->GetForwardVector() * nearZ - toRight - toTop); // Bottom Left

	return res;
}

Vector Camera::GetNearBottomLeftCorner()
{
	float aspect = frameWidth / frameHeight;
	float fovWHalf = fov * 0.5f;
	float deg2rad = 0.01745329251f;

	Vector toRight = gameObject->GetRightVector() * nearZ * tan(fovWHalf * deg2rad) * aspect;
	Vector toTop = gameObject->GetUpVector() * nearZ * tan(fovWHalf * deg2rad);
	return gameObject->GetForwardVector() * nearZ - toRight - toTop; // Bottom Left
}

Vector Camera::GetFarBottomLeftCorner()
{
	float aspect = frameWidth / frameHeight;
	float fovWHalf = fov * 0.5f;
	float deg2rad = 0.01745329251f;

	Vector toRight = gameObject->GetRightVector() * nearZ * tan(fovWHalf * deg2rad) * aspect;
	Vector toTop = gameObject->GetUpVector() * nearZ * tan(fovWHalf * deg2rad);
	Vector bottomLeft = gameObject->GetForwardVector() * nearZ - toRight - toTop;
	float scale = length(bottomLeft) * farZ / nearZ;
	normalize(bottomLeft);
	return bottomLeft * scale;
}

void Camera::FinalDeferredPass(DirectionalLight* light, Color ambientLight)
{
	glUseProgram(finalDeferred);
	int id = glGetUniformLocation(finalDeferred, "colorBuffer");
	if (id >= 0 && colorBuffer >= 0)
	{
		int unit = 0;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		glBindSampler(unit, colorSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(finalDeferred, "normalBuffer");
	if (id >= 0 && normalBuffer >= 0)
	{
		int unit = 1;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, normalBuffer);
		glBindSampler(unit, colorSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(finalDeferred, "depthBuffer");
	if (id >= 0 && depthBuffer >= 0)
	{
		int unit = 2;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glBindSampler(unit, colorSampler);
		glUniform1i(id, unit);
	}

	Vector camPos = this->GetGameObject()->GetPosition();
	Vector lightDir = light->GetGameObject()->GetForwardVector();
	Color lightColor = light->GetColor();
	glUniform3fv(glGetUniformLocation(finalDeferred, "camPos"), 1, &camPos.x);
	glUniform4fv(glGetUniformLocation(finalDeferred, "ambientLight"), 1, &ambientLight.r);
	glUniform3fv(glGetUniformLocation(finalDeferred, "lightDir"), 1, &lightDir.x);
	glUniform4fv(glGetUniformLocation(finalDeferred, "lightColor"), 1, &lightColor.r);
	glUniform1f(glGetUniformLocation(finalDeferred, "lightStrength"), light->GetStrength());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
