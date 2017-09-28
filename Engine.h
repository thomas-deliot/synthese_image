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
#include "Camera.h"

#include <chrono>

class Engine : public App
{
private:
	GameObject* rootObject;
	vector<GameObject*> gameObjects;
	Camera* mainCamera;
	Uint64 lastTime;
	Uint64 newTime;
	Text console;
	unsigned int oldFPSTimer = 0;
	unsigned int oldNewGameTimer = 0;
	unsigned int currentNewGameTimer = 0;
	bool displayWinner = false;
	int winnerID;

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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (int i = 0; i < gameObjects.size(); i++)
		{
			MeshRenderer* renderer = gameObjects[i]->GetComponent<MeshRenderer>();
			if (renderer != nullptr)
				draw(renderer->GetMesh(), gameObjects[i]->GetObjectToWorldMatrix(),
					mainCamera->GetViewMatrix(), mainCamera->GetProjectionMatrix(), renderer->GetTexture());
			/*if (renderer != nullptr)
				renderer->Draw(mainCamera);*/
		}
		GUI();
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
		GameObject* guy = new GameObject();
		guy->SetName("guy");
		MeshRenderer* renderer2 = new MeshRenderer();
		guy->AddComponent(renderer2);
		renderer2->LoadMesh("data/cube.obj");
		renderer2->LoadShader("data/shaders/basic_shader.glsl");
		gameObjects.push_back(guy);
		rootObject->AddChild(guy);
		guy->SetPosition(10.0f, 0.0f, 0.0f);
		guy->SetScale(1.0f, 1.0f, 1.0f);
		renderer2->SetColor(Color(0, 0, 1, 1));

		// Set up terrain
		/*GameObject* terrain = new GameObject();
		terrain->SetName("terrain");
		MeshRenderer* renderer1 = new MeshRenderer();
		GameTerrain* gameTerrain = new GameTerrain("data/terrain/circuit2.png", 15.0f, 400.0f, 100.0f);
		terrain->AddComponent(gameTerrain);
		terrain->AddComponent(renderer1);
		renderer1->LoadTexture("data/terrain/circuit2_color.png");
		renderer1->LoadShader("data/shaders/basic_shader.glsl");
		gameObjects.push_back(terrain);
		rootObject->AddChild(terrain);
		terrain->SetPosition(0.0f, 0.0f, 0.0f);
		terrain->SetScale(1.0f, 1.0f, 1.0f);

		GameObject* terrain2 = new GameObject();
		terrain2->SetName("terrain");
		MeshRenderer* renderer3 = new MeshRenderer();
		GameTerrain* gameTerrain2 = new GameTerrain("data/terrain/circuit2.png", 15.0f, 400.0f, 100.0f);
		terrain2->AddComponent(gameTerrain2);
		terrain2->AddComponent(renderer3);
		renderer3->LoadTexture("data/terrain/circuit2_color.png");
		renderer3->LoadShader("data/shaders/basic_shader.glsl");
		gameObjects.push_back(terrain2);
		rootObject->AddChild(terrain2);
		terrain2->SetPosition(0.0f, 10.0f, 0.0f);
		terrain2->SetScale(1.0f, 1.0f, 1.0f);
		renderer3->SetColor(Color(0, 1, 0, 1));*/

		// Set up camera
		GameObject* cameraObject = new GameObject();
		cameraObject->SetName("cameraObject");
		mainCamera = new Camera();
		cameraObject->AddComponent(mainCamera);
		gameObjects.push_back(cameraObject);
		rootObject->AddChild(cameraObject);
		cameraObject->SetRotation(TQuaternion<float, Vector>(Vector(0, 1, 0), 0.0f));// 1.5708f));
		cameraObject->SetPosition(0.0f, 0.0f, 0.0f);
		//cameraObject->LookAt(Vector(0.0f, 0.0f, 0.0f));
		/*FlyCamera* cameraController = new FlyCamera();
		cameraObject->AddComponent(cameraController);*/
	}

	void GUI()
	{
		clear(console);
		unsigned int currentFPSTimer = SDL_GetTicks();
		printf(console, 0, 0, "FPS: %.1f", 1000.0f / (currentFPSTimer - oldFPSTimer));
		oldFPSTimer = currentFPSTimer;
		draw(console, window_width(), window_height());
	}
};