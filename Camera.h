#pragma once

#include "Component.h"
#include "DirectionalLight.h"

#include "mat.h"
#include "wavefront.h"
#include "texture.h"
#include "program.h"
#include "uniforms.h"
#include <vector>

class Skybox;

using namespace std;

class Camera : public Component
{
private:
	Transform projectionMatrix;
	int frameWidth = 800;
	int frameHeight = 800;
	float nearZ = 0.1f;
	float farZ = 2000.0f;
	float fov = 60.0f;
	GLuint frameBuffer;
	GLuint colorBuffer; // RGB : albedo.rgb / A : roughness
	GLuint normalBuffer; // RGB : normal.xyz / A : metallic
	GLuint depthBuffer;
	GLuint colorSampler;
	GLuint normalSampler;
	GLuint depthSampler;

	// Post effect
	GLuint deferredFinalPass;
	GLuint shaderSSAO;
	GLuint frameBuffer2;
	GLuint colorBuffer2;
	GLuint skyboxSampler;

	// Previous frame for temporal reprojection
	Transform prevProjectionMatrix;
	Transform prevViewMatrix;
	GLuint prevColorBuffer;
	GLuint prevColorSampler;

public:
	void Start()
	{
		SetParameters(frameWidth, frameHeight, fov, nearZ, farZ);
	}

	void OnDestroy()
	{
		glDeleteTextures(1, &colorBuffer);
		glDeleteTextures(1, &depthBuffer);
		glDeleteFramebuffers(1, &frameBuffer);
		glDeleteSamplers(1, &colorSampler);
		glDeleteSamplers(1, &normalSampler);
		glDeleteTextures(1, &colorBuffer2);
		glDeleteFramebuffers(1, &frameBuffer2);
		glDeleteProgram(deferredFinalPass);
		glDeleteTextures(1, &prevColorBuffer);
		glDeleteSamplers(1, &prevColorSampler);
	}

	void LoadDeferredShader(string s)
	{
		deferredFinalPass = read_program(s.c_str());
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

		glGenTextures(1, &prevColorBuffer);
		glBindTexture(GL_TEXTURE_2D, prevColorBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA, frameWidth, frameHeight, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		glGenSamplers(1, &prevColorSampler);
		glSamplerParameteri(prevColorSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(prevColorSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(prevColorSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(prevColorSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		// Normal Buffer setup
		glGenTextures(1, &normalBuffer);
		glBindTexture(GL_TEXTURE_2D, normalBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA16F, frameWidth, frameHeight, 0,
			GL_RGBA, GL_HALF_FLOAT, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		glGenSamplers(1, &normalSampler);
		glSamplerParameteri(normalSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glSamplerParameteri(normalSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(normalSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(normalSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		// Depth Buffer setup
		glGenTextures(1, &depthBuffer);
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_DEPTH_COMPONENT, frameWidth, frameHeight, 0,
			GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
		glGenSamplers(1, &depthSampler);
		glSamplerParameteri(depthSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(depthSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(depthSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(depthSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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

		// Skybox sampler
		glGenSamplers(1, &skyboxSampler);
		glSamplerParameteri(skyboxSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(skyboxSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(skyboxSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(skyboxSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(skyboxSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		// Cleanup
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	void FinishDeferredRendering(DirectionalLight* light, Skybox* skybox)
	{
		BeginPostEffect();
		FinalDeferredPassSSR(light, skybox);
		EndPostEffect();
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

	void FinalDeferredPassSSR(DirectionalLight* light, Skybox* skybox);

	void SSAO();

	void DrawPostEffects()
	{
		glDisable(GL_DEPTH_TEST);

		BeginPostEffect();
		SSAO();
		EndPostEffect();

		// Reset before ending
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorBuffer, 0);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, normalBuffer, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glUseProgram(0);
		glEnable(GL_DEPTH_TEST);
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

	void UpdatePreviousColorBuffer()
	{
		glBindTexture(GL_TEXTURE_2D, prevColorBuffer);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, frameWidth, frameHeight, 0);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, frameWidth, frameHeight);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, prevColorBuffer);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		prevProjectionMatrix = projectionMatrix;
		prevViewMatrix = GetViewMatrix();
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
