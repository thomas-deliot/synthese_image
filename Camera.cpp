#include "Camera.h"
#include "GameObject.h"
#include "Skybox.h"
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

void Camera::FinalDeferredPassSSR(DirectionalLight* light, Skybox* skybox)
{
	glUseProgram(deferredFinalPass);
	int id = glGetUniformLocation(deferredFinalPass, "colorBuffer");
	if (id >= 0 && colorBuffer >= 0)
	{
		int unit = 0;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		glBindSampler(unit, colorSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(deferredFinalPass, "normalBuffer");
	if (id >= 0 && normalBuffer >= 0)
	{
		int unit = 1;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, normalBuffer);
		glBindSampler(unit, normalSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(deferredFinalPass, "depthBuffer");
	if (id >= 0 && depthBuffer >= 0)
	{
		int unit = 2;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glBindSampler(unit, depthSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(deferredFinalPass, "prevColorBuffer");
	if (id >= 0 && prevColorBuffer >= 0)
	{
		int unit = 3;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, prevColorBuffer);
		glBindSampler(unit, prevColorSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(deferredFinalPass, "skybox");
	if (id >= 0 && skybox->GetTexCube() >= 0)
	{
		int unit = 4;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->GetTexCube());
		glBindSampler(unit, skyboxSampler);
		glUniform1i(id, unit);
	}

	Transform trs = Translation(0.5f, 0.5f, 0.0f);
	trs = trs * Scale(0.5f, 0.5f, 1.0f);
	Transform screenScale = Scale(frameWidth, frameHeight, 1.0f);
	Transform projToPixel = screenScale * trs * projectionMatrix;
	Transform invP = projectionMatrix.inverse();
	Transform invV = GetViewMatrix().inverse();
	glUniformMatrix4fv(glGetUniformLocation(deferredFinalPass, "projToPixel"), 1, GL_TRUE, projToPixel.buffer());
	glUniformMatrix4fv(glGetUniformLocation(deferredFinalPass, "invProj"), 1, GL_TRUE, invP.buffer());
	glUniformMatrix4fv(glGetUniformLocation(deferredFinalPass, "prevProj"), 1, GL_TRUE, prevProjectionMatrix.buffer());
	glUniformMatrix4fv(glGetUniformLocation(deferredFinalPass, "invView"), 1, GL_TRUE, invV.buffer());
	glUniformMatrix4fv(glGetUniformLocation(deferredFinalPass, "prevView"), 1, GL_TRUE, prevViewMatrix.buffer());
	glUniformMatrix4fv(glGetUniformLocation(deferredFinalPass, "viewMatrix"), 1, GL_TRUE, GetViewMatrix().buffer());
	glUniform1f(glGetUniformLocation(deferredFinalPass, "nearZ"), nearZ);
	glUniform1f(glGetUniformLocation(deferredFinalPass, "farZ"), farZ);
	vec2 screenSize = vec2(frameWidth, frameHeight);
	glUniform2fv(glGetUniformLocation(deferredFinalPass, "renderSize"), 1, &(screenSize.x));

	Vector camPos = this->GetGameObject()->GetPosition();
	Vector lightDir = light->GetGameObject()->GetForwardVector();
	Color lightColor = light->GetColor();
	glUniform3fv(glGetUniformLocation(deferredFinalPass, "camPos"), 1, &camPos.x);
	glUniform3fv(glGetUniformLocation(deferredFinalPass, "lightDir"), 1, &lightDir.x);
	glUniform4fv(glGetUniformLocation(deferredFinalPass, "lightColor"), 1, &lightColor.r);
	glUniform1f(glGetUniformLocation(deferredFinalPass, "lightStrength"), light->GetStrength());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Camera::SSAO()
{
	glUseProgram(shaderSSAO);
	int id = glGetUniformLocation(shaderSSAO, "colorBuffer");
	if (id >= 0 && colorBuffer >= 0)
	{
		int unit = 0;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		glBindSampler(unit, colorSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(shaderSSAO, "normalBuffer");
	if (id >= 0 && normalBuffer >= 0)
	{
		int unit = 1;
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, normalBuffer);
		glBindSampler(unit, normalSampler);
		glUniform1i(id, unit);
	}
	id = glGetUniformLocation(shaderSSAO, "depthBuffer");
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
	glUniformMatrix4fv(glGetUniformLocation(shaderSSAO, "projToPixel"), 1, GL_TRUE, projToPixel.buffer());
	Transform invP = projectionMatrix.inverse();
	glUniformMatrix4fv(glGetUniformLocation(shaderSSAO, "invProj"), 1, GL_TRUE, invP.buffer());
	glUniformMatrix4fv(glGetUniformLocation(shaderSSAO, "viewMatrix"), 1, GL_TRUE, GetViewMatrix().buffer());
	glUniform1f(glGetUniformLocation(shaderSSAO, "nearZ"), nearZ);
	glUniform1f(glGetUniformLocation(shaderSSAO, "farZ"), farZ);
	vec2 screenSize = vec2(frameWidth, frameHeight);
	glUniform2fv(glGetUniformLocation(shaderSSAO, "renderSize"), 1, &(screenSize.x));

	Transform invV = GetViewMatrix().inverse();
	glUniformMatrix4fv(glGetUniformLocation(shaderSSAO, "invView"), 1, GL_TRUE, invV.buffer());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
