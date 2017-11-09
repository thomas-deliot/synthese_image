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
#include "Skybox.h"

#include <chrono>

class Engine : public App
{
private:
	Uint64 lastTime;
	Uint64 newTime;
	Text console;
	unsigned int oldFPSTimer = 0;
	int frameWidth = 600;
	int frameHeight = 600;

	// Scene setup
	GameObject* rootObject;
	vector<GameObject*> gameObjects;
	Camera* mainCamera;
	DirectionalLight* mainLight;
	Color ambientLight = Color(0.1f, 0.1f, 0.1f, 0.1f);
	Skybox* skybox;

public:
	Engine() : App(600, 600) {}

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
				components[j]->Start();
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
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, mainCamera->GetNormalBuffer(), 0);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, mainCamera->GetDepthBuffer(), 0);
		glViewport(0, 0, frameWidth, frameHeight);
		glClearColor(1, 1, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		skybox->Draw(mainCamera);
		for (int i = 0; i < gameObjects.size(); i++)
		{
			MeshRenderer* renderer = gameObjects[i]->GetComponent<MeshRenderer>();
			if (renderer != nullptr)
				renderer->Draw(mainCamera, mainLight, ambientLight);
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glUseProgram(0);

		// Draw post effects
		mainCamera->DrawPostEffects();

		// Blit to screen
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
		/*GameObject* guy = new GameObject();
		guy->SetName("guy");
		MeshRenderer* renderer = new MeshRenderer();
		guy->AddComponent(renderer);
		renderer->LoadMesh("data/bigguy.obj");
		renderer->LoadShader("m2tp/Shaders/basic_shader.glsl");
		renderer->LoadTexture("data/debug2x2red.png");
		gameObjects.push_back(guy);
		rootObject->AddChild(guy);
		guy->SetPosition(10.0f, 0.0f, 0.0f);
		renderer->SetColor(Color(1.0, 1.0, 1.0, 1.0));
		RotateObjectMouse* rotater = new RotateObjectMouse();*/
		//guy->AddComponent(rotater);

		/*GameObject* guy4 = new GameObject();
		guy4->SetName("guy4");
		MeshRenderer* renderer5 = new MeshRenderer();
		guy4->AddComponent(renderer5);
		renderer5->LoadMesh("data/bigguy.obj");
		renderer5->LoadShader("m2tp/Shaders/basic_shader.glsl");
		renderer5->LoadTexture("data/debug2x2red.png");
		gameObjects.push_back(guy4);
		rootObject->AddChild(guy4);
		guy4->SetPosition(10.0f, 0.0f, 0.0f);
		renderer5->SetColor(Color(1.0, 1.0, 1.0, 1.0));*/

		GameObject* cube1 = new GameObject();
		cube1->SetName("cube1");
		MeshRenderer* renderer2 = new MeshRenderer();
		cube1->AddComponent(renderer2);
		renderer2->LoadMesh("data/cube.obj");
		renderer2->LoadShader("m2tp/Shaders/basic_shader.glsl");
		renderer2->LoadTexture("data/debug2x2red.png");
		gameObjects.push_back(cube1);
		rootObject->AddChild(cube1);
		cube1->SetPosition(0.0f, -10.0f, 0.0f);
		cube1->SetScale(40.0f, 1.0f, 40.0f);
		renderer2->SetColor(Color(1.0, 1.0, 1.0, 1.0));

		GameObject* cube2 = new GameObject();
		cube2->SetName("cube2");
		MeshRenderer* renderer3 = new MeshRenderer();
		cube2->AddComponent(renderer3);
		renderer3->LoadMesh("data/cube.obj");
		renderer3->LoadShader("m2tp/Shaders/basic_shader.glsl");
		renderer3->LoadTexture("data/rainbow.jpg");
		gameObjects.push_back(cube2);
		rootObject->AddChild(cube2);
		cube2->SetPosition(0.0f, 10.0f, -20.0f);
		cube2->SetScale(40.0f, 1.0f, 40.0f);
		cube2->RotateAround(Vector(1, 0, 0), -90.0f);
		renderer3->SetColor(Color(1.0, 1.0, 1.0, 1.0));

		GameObject* cube3 = new GameObject();
		cube3->SetName("cube3");
		MeshRenderer* renderer4 = new MeshRenderer();
		cube3->AddComponent(renderer4);
		renderer4->LoadMesh("data/cube.obj");
		renderer4->LoadShader("m2tp/Shaders/basic_shader.glsl");
		renderer4->LoadTexture("data/debug2x2red.png");
		gameObjects.push_back(cube3);
		rootObject->AddChild(cube3);
		cube3->SetPosition(20.0f, 10.0f, 0.0f);
		cube3->SetScale(40.0f, 1.0f, 40.0f);
		cube3->RotateAround(Vector(1, 0, 0), -90.0f);
		cube3->RotateAround(Vector(0, 1, 0), -90.0f);
		renderer4->SetColor(Color(1.0, 1.0, 1.0, 1.0));

		// Set up light
		GameObject* lightObject = new GameObject();
		lightObject->SetName("lightObject");
		lightObject->SetPosition(0.0f, 0.0f, 0.0f);
		lightObject->RotateAroundRadian(Vector(0, 1, 0), 180);
		lightObject->RotateAroundRadian(Vector(1, 0, 0), 45);
		mainLight = new DirectionalLight(1.0f, White());
		lightObject->AddComponent(mainLight);
		rootObject->AddChild(lightObject);
		gameObjects.push_back(lightObject);

		// Set up camera
		GameObject* cameraObject = new GameObject();
		cameraObject->SetName("cameraObject");
		mainCamera = new Camera();
		cameraObject->AddComponent(mainCamera);
		gameObjects.push_back(cameraObject);
		rootObject->AddChild(cameraObject);
		cameraObject->SetPosition(0.0f, 0.0f, 35.0f);
		//cameraObject->RotateAround(cameraObject->GetRightVector(), 90);
		mainCamera->SetupFrameBuffer(frameWidth, frameHeight);
		FlyCamera* flyCam = new FlyCamera();
		cameraObject->AddComponent(flyCam);

		// Set up skybox
		skybox = new Skybox();
		lightObject->AddComponent(skybox);
		skybox->CreateCubeMap("m2tp/Scene/Skybox1/posz.jpg",
			"m2tp/Scene/Skybox1/negz.jpg",
			"m2tp/Scene/Skybox1/posy.jpg",
			"m2tp/Scene/Skybox1/negy.jpg",
			"m2tp/Scene/Skybox1/posx.jpg",
			"m2tp/Scene/Skybox1/negx.jpg");
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