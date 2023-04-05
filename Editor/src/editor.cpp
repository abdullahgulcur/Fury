#include "pch.h"
#include "editor.h"

namespace Editor{


	Editor::Editor() {

		menu = new Menu();
		sceneCamera = new SceneCamera();
		//gizmo = new Gizmo();
		renderer = new Renderer();

		std::cout << "Editor started." << std::endl;
	}

	void Editor::init() {

		renderer->init();
		menu->init();
		sceneCamera->init();
	}

	void Editor::update(float dt) {

		menu->update();
		sceneCamera->update(dt);
		renderer->update(dt);
	}

	Editor* Editor::getInstance() {

		Editor::instance = new Editor;
		return instance;
	}

}