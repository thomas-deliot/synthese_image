#pragma once

#include "mat.h"
#include "wavefront.h"
#include "texture.h"
#include "Text.h"

#include "orbiter.h"
#include "draw.h"
#include "app.h"

#include "GameObject.h"
#include "MeshRenderer.h"
#include "GameTerrain.h"
#include "FlyCamera.h"
#include "RotateObjectMouse.h"
#include "Camera.h"
#include "DirectionalLight.h"

#include <chrono>

class Engine : public App
{
private:
	Uint64 lastTime;
	Uint64 newTime;
	Text console;
	unsigned int oldFPSTimer = 0;
	int frameWidth = 800;
	int frameHeight = 800;

	// Scene setup
	GameObject* rootObject;
	vector<GameObject*> gameObjects;
	Camera* mainCamera;
	DirectionalLight* mainLight;
	Color ambientLight = Color(0.1f, 0.1f, 0.1f, 0.1f);

public:
	Engine() : App(800, 800) {}

	int init()
	{
		// Create scene root object
		rootObject = new GameObject();
		gameObjects.push_back(rootObject);

		// Init GranPourrismo game
		InitScene();
		console = create_text();

		// On Start 
		for (int i = 0; i < gameObjects.size(); i++)
		{
			vector<Component*> components = gameObjects[i]->GetAllComponents();
			for (int j = 0; j < components.size(); j++)
				components[j]->OnStart();
		}

		// etat openGL par defaut
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);       // couleur par defaut de la fenetre

		glClearDepth(1.0f);                         // profondeur par defaut
		glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
		glEnable(GL_DEPTH_TEST);                    // activer le ztest
		glFrontFace(GL_CCW);

		lastTime = SDL_GetPerformanceCounter();
		return 0;   // ras, pas d'erreur
	}

	int quit()
	{
		for (int i = 0; i < gameObjects.size(); i++)
		{
			vector<Component*> components = gameObjects[i]->GetAllComponents();
			for (int j = 0; j < components.size(); j++)
			{
				components[j]->OnDestroy();
				delete components[j];
			}
			delete gameObjects[i];
		}
		release_text(console);
		return 0;
	}

	int render()
	{
		// Draw scene
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mainCamera->GetFrameBuffer());
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mainCamera->GetColorBuffer(), 0);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, mainCamera->GetDepthBuffer(), 0);
		glViewport(0, 0, frameWidth, frameHeight);
		glClearColor(1, 1, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (int i = 0; i < gameObjects.size(); i++)
		{
			MeshRenderer* renderer = gameObjects[i]->GetComponent<MeshRenderer>();
			if (renderer != nullptr)
				renderer->Draw(mainCamera, mainLight, ambientLight);
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glUseProgram(0);

		// Draw post effects
		mainCamera->DrawPostEffect();

		// Draw to screen
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mainCamera->GetFrameBuffer());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glViewport(0, 0, window_width(), window_height());
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glBlitFramebuffer(
			0, 0, frameWidth, frameHeight,
			0, 0, frameWidth, frameHeight,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);

		DisplayGUI();
		return 1;
	}

	int update(const float time, const float delta)
	{
		newTime = SDL_GetPerformanceCounter();
		float delta2 = (double)((newTime - lastTime) * 1000) / SDL_GetPerformanceFrequency();

		for (int i = 0; i < gameObjects.size(); i++)
		{
			gameObjects[i]->UpdateTransformIfNeeded();
			vector<Component*> components = gameObjects[i]->GetAllComponents();
			for (int j = 0; j < components.size(); j++)
				components[j]->Update(delta2);
		}

		lastTime = SDL_GetPerformanceCounter();
		return 0;
	}


	void InitScene()
	{
		GameObject* guy4 = new GameObject();
		guy4->SetName("guy4");
		MeshRenderer* renderer5 = new MeshRenderer();
		guy4->AddComponent(renderer5);
		renderer5->LoadMesh("data/bigguy.obj");
		renderer5->LoadShader("m2tp/Shaders/basic_shader.glsl");
		renderer5->LoadTexture("data/debug2x2red.png");
		gameObjects.push_back(guy4);
		rootObject->AddChild(guy4);
		guy4->SetPosition(0.0f, 0.0f, 0.0f);
		renderer5->SetColor(Color(1.0, 1.0, 1.0, 1.0));
		RotateObjectMouse* rotater = new RotateObjectMouse();
		guy4->AddComponent(rotater);

		// Set up light
		GameObject* lightObject = new GameObject();
		lightObject->SetName("lightObject");
		lightObject->SetPosition(0.0f, 0.0f, 0.0f);
		lightObject->RotateAroundRadian(Vector(0, 1, 0), 180);
		lightObject->RotateAroundRadian(Vector(1, 0, 0), 45);
		mainLight = new DirectionalLight(1.0f, White());
		lightObject->AddComponent(mainLight);

		// Set up camera
		GameObject* cameraObject = new GameObject();
		cameraObject->SetName("cameraObject");
		mainCamera = new Camera();
		cameraObject->AddComponent(mainCamera);
		gameObjects.push_back(cameraObject);
		rootObject->AddChild(cameraObject);
		cameraObject->SetPosition(0.0f, 0.0f, 35.0f);
		mainCamera->SetupFrameBuffer(frameWidth, frameHeight);
		//FlyCamera* flyCam = new FlyCamera();
		//cameraObject->AddComponent(flyCam);
	}

	void DisplayGUI()
	{
		clear(console);
		unsigned int currentFPSTimer = SDL_GetTicks();
		printf(console, 0, 0, "FPS: %.1f", 1000.0f / (currentFPSTimer - oldFPSTimer));
		oldFPSTimer = currentFPSTimer;
		draw(console, window_width(), window_height());
	}
};