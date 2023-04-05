#include "pch.h"
#include "scenemanager.h"
#include "filesystem/scenefile.h"
#include "scene.h"
#include "filesystem.h"
#include "core.h"

namespace Fury {

	SceneManager::SceneManager() {

	}

	SceneManager::~SceneManager() {

	}

	void SceneManager::init() {

		std::ifstream file(Core::instance->fileSystem->getSceneManagerPath());

		if (file.fail())
			return;

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* active_scene_node = NULL;
		rapidxml::xml_node<>* scenes_node = NULL;

		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');

		doc.parse<0>(&buffer[0]);

		active_scene_node = doc.first_node("ActiveScene");
		std::string activeSceneName = active_scene_node->first_attribute("Name")->value();

		SceneManager::loadScene(activeSceneName);
	}

	void SceneManager::loadScene(std::string sceneName) {

		auto file = sceneFiles.find(sceneName);

		if (file != sceneFiles.end()) {

			if (currentScene)
				delete currentScene;

			auto sceneFile = Core::instance->fileSystem->fileToSceneFile.find(file->second);
			if (sceneFile != Core::instance->fileSystem->fileToSceneFile.end()) {

				currentSceneFile = file->second;
				currentScene = new Scene(sceneName);
				sceneFile->second->loadEntities(file->second->path);
			}

			if (currentScene->primaryCamera)
				currentScene->primaryCamera->resetResolution(Core::instance->renderer->width, Core::instance->renderer->height);
		}
		else {

		}
	}

	void SceneManager::renameCurrentScene(std::string sceneName) {

		std::string oldSceneName = currentScene->name;
		currentScene->name = sceneName;
		auto nodeHandler = sceneFiles.extract(oldSceneName);
		nodeHandler.key() = sceneName;
		sceneFiles.insert(std::move(nodeHandler));
	}

	File* SceneManager::getActiveSceneFile() {
		return currentSceneFile;
	}


	void SceneManager::restartCurrentScene() {

		std::string name = currentScene->name;
		SceneManager::loadScene(name);
	}

	//void SceneManager::loadSceneFromEditor(File* file) {

	//	auto sceneFile = Core::instance->fileSystem->fileToSceneFile.find(file);
	//	if (sceneFile != Core::instance->fileSystem->fileToSceneFile.end())
	//		sceneFile->second->loadEntities(file->path);

	//	if(currentScene->primaryCamera)
	//		currentScene->primaryCamera->resetResolution(Core::instance->renderer->width, Core::instance->renderer->height);
	//}

	void SceneManager::deleteCurrentScene() {

		if (!currentScene)
			return;

		delete currentScene;
		currentScene = NULL;
		currentSceneFile = NULL;
	}

	void SceneManager::deleteScene(std::string sceneName) {

	}

	void SceneManager::saveSceneManagerFile() {

		std::ifstream file(Core::instance->fileSystem->getSceneManagerPath());

		if (file.fail())
			return;

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* active_scene_node = doc.allocate_node(rapidxml::node_element, "ActiveScene");
		if(currentScene)
			active_scene_node->append_attribute(doc.allocate_attribute("Name", doc.allocate_string(currentScene->name.c_str())));
		else
			active_scene_node->append_attribute(doc.allocate_attribute("Name", doc.allocate_string("null")));

		doc.append_node(active_scene_node);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(Core::instance->fileSystem->getSceneManagerPath());
		file_stored << doc;
		file_stored.close();
		doc.clear();
	}

}