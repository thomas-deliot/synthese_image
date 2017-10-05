#pragma once

#include "Component.h"
#include "mat.h"
#include "program.h"

class Camera : public Component
{
private:
	//Transform projectionMatrix = Orthographic(150.0f, 150.0f, 0.1f, 1000.0f);
	Transform projectionMatrix = Perspective(60.0f, 1.0f, 0.1f, 1000.0f);
	int frameWidth = 800;
	int frameHeight = 800;
	GLuint frameBuffer;
	GLuint colorBuffer;
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
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER,  /* attachment */ GL_DEPTH_ATTACHMENT, /* texture */ depthBuffer, /* mipmap level */ 0);

		// Fragment shader output
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, buffers);


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
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER,  /* attachment */ GL_COLOR_ATTACHMENT0, /* texture */ colorBuffer2, /* mipmap level */ 0);

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
};
