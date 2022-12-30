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

		HINSTANCE hDLL = LoadLibrary("C:/Projects/Fury/x64/Debug/Game.dll");
		if (NULL != hDLL)
		{
			lpfnDllFuncUpdate = (LPFNDLLFUNC)GetProcAddress(hDLL, "update");
			lpfnDllFuncStart = (LPFNDLLFUNC)GetProcAddress(hDLL, "start");
		}
	}

	void Core::init() {

		//scene = new Scene();
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

		fileSystem->onUpdate();
		glfwContext->update();
		renderer->update();
	}

	float Core::getCurrentTime() {

		return glfwGetTime();
	}

	Core* Core::getInstance() {

		Core::instance = new Core;
		return instance;
	}
}
