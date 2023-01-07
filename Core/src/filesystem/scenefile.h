#pragma once

#include "rapidXML/rapidxml_print.hpp"
#include "rapidXML/rapidxml.hpp"

#include "entity.h"
#include "component/transform.h"
#include "component/terrain.h"

#include "filesystem.h"

namespace Fury {

	// buradaki fonksiyonlar scene manager e tasinabilir bence? 
	class __declspec(dllexport) SceneFile {

	private:

	public:

		SceneFile(const char* scenepath);
		~SceneFile();
		bool saveEntities(std::string filePath);
		void saveEntitiesRecursively(Transform* parent, rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entityNode);
		void loadEntities(std::string filePath);
		void renameFile(std::string path, std::string name);
		void loadEntitiesRecursively(rapidxml::xml_node<>* parentNode, Entity* parent);
		bool saveTransformComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Transform* transform);
		bool loadTransformComponent(Entity* ent, rapidxml::xml_node<>* entNode);
		bool saveMeshRendererComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, MeshRenderer* meshRenderer);
		bool loadMeshRendererComponent(Entity* ent, rapidxml::xml_node<>* entNode);
		bool saveGameCameraComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, GameCamera* camera);
		bool loadGameCameraComponent(Entity* ent, rapidxml::xml_node<>* entNode);
		bool saveTerrainComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Terrain* terrain);
		bool loadTerrainComponent(Entity* ent, rapidxml::xml_node<>* entNode);
	};
}