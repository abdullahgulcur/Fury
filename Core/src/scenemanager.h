#pragma once

#include "entity.h"
#include "component/gamecamera.h"

namespace Fury {
	
	class Core;
	class Scene;
	class SceneFile;
	class File;

	class __declspec(dllexport) SceneManager {

	private:

	public:

		Scene* currentScene = NULL;
		File* currentSceneFile;
		std::map<std::string, File*> sceneFiles;

		SceneManager();
		~SceneManager();
		void init();
		void loadScene(std::string sceneName);
		void renameCurrentScene(std::string sceneName);
		void renameScene(std::string oldSceneName, std::string newSceneName);
		File* getActiveSceneFile();
		void restartCurrentScene();
		void deleteScene(std::string sceneName);
		void deleteCurrentScene();
		void saveSceneManagerFile();
	};
}