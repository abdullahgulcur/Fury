#include "pch.h"
#include "editor.h"

namespace Editor{

	Editor::Editor() {

		menu = new Menu();
		sceneCamera = new SceneCamera();

		std::cout << "Editor started." << std::endl;
	}

	void Editor::init() {

		menu->init();
		sceneCamera->init();
	}

	void Editor::update(float dt) {

		menu->update();
		sceneCamera->update(dt);

		// scene update
		SceneCamera* camera = Editor::instance->sceneCamera;
		CameraInfo cameraInfo;
		cameraInfo.FBO = camera->FBO;
		cameraInfo.VP = camera->projectionViewMatrix;
		cameraInfo.projection = camera->ProjectionMatrix;
		cameraInfo.view = camera->ViewMatrix;
		cameraInfo.camPos = camera->position;
		cameraInfo.width = camera->width;
		cameraInfo.height = camera->height;
		for (int i = 0; i < 6; i++)
			cameraInfo.planes[i] = camera->planes[i];

		Core::instance->renderer->cameraInfo = cameraInfo;
		Core::instance->renderer->update(dt);
	}

	Editor* Editor::getInstance() {

		Editor::instance = new Editor;
		return instance;
	}
}