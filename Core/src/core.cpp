#include "pch.h"
#include "core.h"

namespace Fury {

	Core::Core() {

		std::cout << "Fury started." << std::endl;

		glfwContext = new GlfwContext();
		glewContext = new GlewContext();
		renderer = new Renderer();
		fileSystem = new FileSystem();
		sceneManager = new SceneManager();
		input = new Input();

		HINSTANCE hDLL = LoadLibrary("C:/Projects/Fury/x64/Debug/Game.dll");
		if (NULL != hDLL)
		{
			lpfnDllFuncUpdate = (LPFNDLLFUNC)GetProcAddress(hDLL, "update");
			lpfnDllFuncStart = (LPFNDLLFUNC)GetProcAddress(hDLL, "start");
		}
	}

	void Core::init() {

		renderer->init();
		fileSystem->init();
		sceneManager->init();
	}

	void Core::startGame() {

		lpfnDllFuncStart((UINT*)Fury::Core::instance);
	}

	void Core::updateGame(float dt) {

		lpfnDllFuncUpdate((UINT*)Fury::Core::instance);
	}

	void Core::update(float dt) {

		fileSystem->onUpdate(); // todo: sadece editor modunda calismali
		glfwContext->update(dt); // todo: sadece editor modunda calismali

		if (!sceneManager->currentScene)
			return;

		if (!sceneManager->currentScene->primaryCamera)
			return;

		sceneManager->currentScene->update(dt);

		// RENDERING SECTION
		GameCamera* camera = sceneManager->currentScene->primaryCamera;
		CameraInfo cameraInfo;
		cameraInfo.FBO = camera->FBO;
		cameraInfo.VP = camera->projectionViewMatrix;
		cameraInfo.projection = camera->projectionMatrix;
		cameraInfo.view = camera->viewMatrix;
		cameraInfo.camPos = camera->position;
		cameraInfo.width = camera->width;
		cameraInfo.height = camera->height;
		for(int i = 0; i < 6; i++)
			cameraInfo.planes[i] = camera->planes[i];

		Core::instance->renderer->cameraInfo = cameraInfo;
		renderer->update(dt);
	}

	float Core::getCurrentTime() {

		return glfwGetTime();
	}

	Core* Core::getInstance() {

		Core::instance = new Core;
		return instance;
	}
}
