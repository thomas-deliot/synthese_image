#pragma once

#include "Component.h"
#include "mat.h"
#include "program.h"
#include <vector>

using namespace std;

class Camera : public Component
{
private:
	//Transform projectionMatrix = Orthographic(150.0f, 150.0f, 0.1f, 1000.0f);
	Transform projectionMatrix;;
	int frameWidth = 800;
	int frameHeight = 800;
	float nearZ = 0.1f;
	float farZ = 100.0f;
	float fov = 60.0f;
	GLuint frameBuffer;
	GLuint colorBuffer;
	GLuint normalBuffer;
	GLuint depthBuffer;
	GLuint colorSampler;

	// Post effect
	GLuint postfxProgram;
	GLuint frameBuffer2;
	GLuint colorBuffer2;

public:
	void Start()
	{
		postfxProgram = read_program("m2tp/Shaders/ssr_fx.glsl");
		SetParameters(frameWidth, frameHeight, fov, nearZ, farZ);
	}

	void OnDestroy()
	{
		glDeleteTextures(1, &colorBuffer);
		glDeleteTextures(1, &depthBuffer);
		glDeleteFramebuffers(1, &frameBuffer);
		glDeleteSamplers(1, &colorSampler);
		glDeleteTextures(1, &colorBuffer2);
		glDeleteFramebuffers(1, &frameBuffer2);
		glDeleteProgram(postfxProgram);
	}

	void SetParameters(const float width, const float height, const float fov, const float nearZ, const float farZ)
	{
		projectionMatrix = Perspective(fov, width / height, nearZ, farZ);
		//projectionMatrix = GetOrthographicMatrix(150.0f, 150.0f, 0.1f, 1000.0f);
	}

	void SetupFrameBuffer(int width, int height)
	{
		frameWidth = width;
		frameHeight = height;

		// Color Buffer setup
		glGenTextures(1, &colorBuffer);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA, frameWidth, frameHeight, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		glGenSamplers(1, &colorSampler);
		glSamplerParameteri(colorSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glSamplerParameteri(colorSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(colorSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(colorSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		// Normal Buffer setup
		glGenTextures(1, &normalBuffer);
		glBindTexture(GL_TEXTURE_2D, normalBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA, frameWidth, frameHeight, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Depth Buffer setup
		glGenTextures(1, &depthBuffer);
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_DEPTH_COMPONENT, frameWidth, frameHeight, 0,
			GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

		// Frame Buffer setup
		glGenFramebuffers(1, &frameBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER,  /* attachment */ GL_COLOR_ATTACHMENT0, /* texture */ colorBuffer, /* mipmap level */ 0);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER,  /* attachment */ GL_COLOR_ATTACHMENT1, /* texture */ normalBuffer, /* mipmap level */ 0);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER,  /* attachment */ GL_DEPTH_ATTACHMENT, /* texture */ depthBuffer, /* mipmap level */ 0);

		// Fragment shader output
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, buffers);


		// Color Buffer 2 setup
		glGenTextures(1, &colorBuffer2);
		glBindTexture(GL_TEXTURE_2D, colorBuffer2);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA, frameWidth, frameHeight, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Frame Buffer 2 setup
		glGenFramebuffers(1, &frameBuffer2);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer2);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, /* attachment */ GL_COLOR_ATTACHMENT0, /* texture */ colorBuffer2, /* mipmap level */ 0);

		// Fragment shader output
		GLenum buffers2[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, buffers2);


		// Cleanup
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	void BeginPostEffect()
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer2);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorBuffer2, 0);
	}

	void EndPostEffect()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBuffer2);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer);
		glViewport(0, 0, frameWidth, frameHeight);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glBlitFramebuffer(
			0, 0, frameWidth, frameHeight,
			0, 0, frameWidth, frameHeight,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

	void DrawPostEffects()
	{
		glDisable(GL_DEPTH_TEST);

		BeginPostEffect();
		DrawScreenSpaceReflections();
		EndPostEffect();

		// Reset before ending
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorBuffer, 0);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, normalBuffer, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glUseProgram(0);
		glEnable(GL_DEPTH_TEST);
	}

	void DrawScreenSpaceReflections()
	{
		glUseProgram(postfxProgram);
		int id = glGetUniformLocation(postfxProgram, "colorBuffer");
		if (id >= 0 && colorBuffer >= 0)
		{
			int unit = 0;
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, colorBuffer);
			glBindSampler(unit, 0);
			glUniform1i(id, unit);
		}
		id = glGetUniformLocation(postfxProgram, "normalBuffer");
		if (id >= 0 && normalBuffer >= 0)
		{
			int unit = 1;
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, normalBuffer);
			glBindSampler(unit, 1);
			glUniform1i(id, unit);
		}
		id = glGetUniformLocation(postfxProgram, "depthBuffer");
		if (id >= 0 && depthBuffer >= 0)
		{
			int unit = 2;
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, depthBuffer);
			glBindSampler(unit, 1);
			glUniform1i(id, unit);
		}

		Transform trs = Translation(0.5f, 0.5f, 0.0f);
		trs = trs * Scale(0.5f, 0.5f, 1.0f);
		Transform screenScale = Scale(frameWidth, frameHeight, 1.0f);
		Transform projToPixel = screenScale * trs * projectionMatrix;

		glUniformMatrix4fv(glGetUniformLocation(postfxProgram, "projToPixel"), 1, GL_TRUE, projToPixel.buffer());
		Transform invP = projectionMatrix.inverse();
		glUniformMatrix4fv(glGetUniformLocation(postfxProgram, "viewMatrix"), 1, GL_TRUE, GetViewMatrix().buffer());
		glUniform1f(glGetUniformLocation(postfxProgram, "nearZ"), nearZ);
		glUniform1f(glGetUniformLocation(postfxProgram, "farZ"), farZ);
		vec2 screenSize = vec2(frameWidth, frameHeight);
		glUniform2fv(glGetUniformLocation(postfxProgram, "renderSize"), 1, &(screenSize.x));

		Vector near = GetNearBottomLeftCorner();
		near = GetViewMatrix()(near);
		Vector far = GetFarBottomLeftCorner();
		far = GetViewMatrix()(far);
		glUniform3fv(glGetUniformLocation(postfxProgram, "nearBottomLeft"), 1, &(near.x));
		glUniform3fv(glGetUniformLocation(postfxProgram, "farBottomLeft"), 1, &(far.x));

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}




	GLuint GetFrameBuffer()
	{
		return frameBuffer;
	}

	GLuint GetColorBuffer()
	{
		return colorBuffer;
	}

	GLuint GetNormalBuffer()
	{
		return normalBuffer;
	}

	GLuint GetDepthBuffer()
	{
		return depthBuffer;
	}

	/*!
	*  \brief Récupère la matrice de projection.
	*/
	Transform GetProjectionMatrix()
	{
		return projectionMatrix;
	}

	/*!
	*  \brief Récupère l'inverse de la matrice du monde.
	*/
	Transform GetViewMatrix();

	/*!
	*  \brief Récupère la matrice de projection orthographique.
	*/
	Transform GetOrthographicMatrix(const float width, const float height, const float znear, const float zfar)
	{
		float w = width / 2.f;
		float h = height / 2.f;

		return Transform(
			1 / w, 0, 0, 0,
			0, 1 / h, 0, 0,
			0, 0, -2 / (zfar - znear), -(zfar + znear) / (zfar - znear),
			0, 0, 0, 1);
	}

	vector<Vector> GetFrustumNearCorners();
	Vector GetNearBottomLeftCorner();
	Vector GetFarBottomLeftCorner();
};
