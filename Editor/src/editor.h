#pragma once

#include "core.h"
#include "menu.h"
#include "scenecamera.h"
#include "gizmo.h"
#include "renderer.h"

//namespace Fury {
//	class Renderer;
//}

using namespace Fury;

namespace Editor {

	class Editor {

	private:

	public:

		bool gameStarted = false; // temp
		Menu* menu = NULL;
		SceneCamera* sceneCamera = NULL; // mevcutta preogram acilir acilmaz baslatiliyor. Scenefile actiginda acmali
		//Gizmo* gizmo = NULL;
		//Renderer* renderer = NULL;
		static Editor* instance;

		Editor();
		void init();
		void update(float dt);
		static Editor* getInstance();
	};
}