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
		glBindSampler(unit, normalSampler);
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
	Transform invP = projectionMatrix.inverse();
	Transform invV = GetViewMatrix().inverse();
	glUniformMatrix4fv(glGetUniformLocation(finalDeferred, "invProj"), 1, GL_TRUE, invP.buffer());
	glUniformMatrix4fv(glGetUniformLocation(finalDeferred, "invView"), 1, GL_TRUE, invV.buffer());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Camera::FinalDeferredPassSSR(DirectionalLight* light, Color ambientLight)
{
	glUseProgram(finalDeferredSSR);
	int id = glGetUniformLocation(finalDeferredSSR, "colorBuffer");
	if (id >= 0 && colorBuffer >= 0)
	{
		int unit = 0;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		glBindSampler(unit, colorSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(finalDeferredSSR, "normalBuffer");
	if (id >= 0 && normalBuffer >= 0)
	{
		int unit = 1;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, normalBuffer);
		glBindSampler(unit, normalSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(finalDeferredSSR, "depthBuffer");
	if (id >= 0 && depthBuffer >= 0)
	{
		int unit = 2;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glBindSampler(unit, colorSampler);
		glUniform1i(id, unit);
	}

	Transform trs = Translation(0.5f, 0.5f, 0.0f);
	trs = trs * Scale(0.5f, 0.5f, 1.0f);
	Transform screenScale = Scale(frameWidth, frameHeight, 1.0f);
	Transform projToPixel = screenScale * trs * projectionMatrix;
	glUniformMatrix4fv(glGetUniformLocation(finalDeferredSSR, "projToPixel"), 1, GL_TRUE, projToPixel.buffer());
	Transform invP = projectionMatrix.inverse();
	glUniformMatrix4fv(glGetUniformLocation(finalDeferredSSR, "invProj"), 1, GL_TRUE, invP.buffer());
	glUniformMatrix4fv(glGetUniformLocation(finalDeferredSSR, "viewMatrix"), 1, GL_TRUE, GetViewMatrix().buffer());
	glUniform1f(glGetUniformLocation(finalDeferredSSR, "nearZ"), nearZ);
	glUniform1f(glGetUniformLocation(finalDeferredSSR, "farZ"), farZ);
	vec2 screenSize = vec2(frameWidth, frameHeight);
	glUniform2fv(glGetUniformLocation(finalDeferredSSR, "renderSize"), 1, &(screenSize.x));

	Vector camPos = this->GetGameObject()->GetPosition();
	Vector lightDir = light->GetGameObject()->GetForwardVector();
	Color lightColor = light->GetColor();
	glUniform3fv(glGetUniformLocation(finalDeferredSSR, "camPos"), 1, &camPos.x);
	glUniform4fv(glGetUniformLocation(finalDeferredSSR, "ambientLight"), 1, &ambientLight.r);
	glUniform3fv(glGetUniformLocation(finalDeferredSSR, "lightDir"), 1, &lightDir.x);
	glUniform4fv(glGetUniformLocation(finalDeferredSSR, "lightColor"), 1, &lightColor.r);
	glUniform1f(glGetUniformLocation(finalDeferredSSR, "lightStrength"), light->GetStrength());
	Transform invV = GetViewMatrix().inverse();
	glUniformMatrix4fv(glGetUniformLocation(finalDeferredSSR, "invView"), 1, GL_TRUE, invV.buffer());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
