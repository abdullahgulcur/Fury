#pragma once

#include "glewcontext.h"
#include "glfwcontext.h"
#include "filesystem.h"
#include "scenemanager.h"
#include "saveloadsystem.h"
#include "renderer.h"

typedef HRESULT(CALLBACK* LPFNDLLFUNC)(UINT*);

namespace Fury {

	class __declspec(dllexport) Core {

	private:

	public:

		LPFNDLLFUNC lpfnDllFuncStart = NULL;
		LPFNDLLFUNC lpfnDllFuncUpdate = NULL;

		static Core* instance;
		GlfwContext* glfwContext = NULL;
		GlewContext* glewContext = NULL;
		FileSystem* fileSystem = NULL;
		SceneManager* sceneManager = NULL;
		Renderer* renderer = NULL;

		Core();
		void init();
		void update(float dt);
		float getCurrentTime();
		void startGame();
		void updateGame(float dt);
		static Core* getInstance();

	};
}