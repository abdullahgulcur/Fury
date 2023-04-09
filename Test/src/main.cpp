#include "pch.h"
#include "core.h"
#include "editor.h"

#ifdef EDITOR_MODE
Editor::Editor* Editor::Editor::instance;
#endif

Fury::Core* Fury::Core::instance;

int main() {

	std::cout << "Program started." << std::endl;

	Fury::Core::instance = Fury::Core::getInstance();
	Fury::Core::instance->init();

#ifndef EDITOR_MODE
	Fury::Core::instance->startGame();
#endif


#ifdef EDITOR_MODE
	Editor::Editor::instance = Editor::Editor::getInstance();
	Editor::Editor::instance->init();
#endif

	float time = 0;
	do {

		float currentTime = (float)glfwGetTime();
		float dt = currentTime - time;
		time = currentTime;

		Fury::Core::instance->update(dt);

#ifdef EDITOR_MODE
		Editor::Editor::instance->update(dt);
#endif

#ifdef EDITOR_MODE
		if (Editor::Editor::instance->gameStarted)
			Fury::Core::instance->updateGame(dt);
#else
		Fury::Core::instance->updateGame(dt);
#endif

		Fury::Core::instance->glfwContext->end(); // basa alinsa ??

	} while (Fury::Core::instance->glfwContext->getOpen());

	Fury::Core::instance->glfwContext->terminateGLFW();

#ifdef EDITOR_MODE
	Editor::Editor::instance->sceneCamera->save(Fury::Core::instance->fileSystem->getSceneCameraPath());
#endif

	//delete Editor::Editor::instance->renderer;

	return 0;
}