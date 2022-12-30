#pragma once

#include "rapidXML/rapidxml_print.hpp"
#include "rapidXML/rapidxml.hpp"

#include "entity.h"
#include "component/transform.h"
//#include "scenecamera.h"

namespace Fury {

	class Core;

	class __declspec(dllexport) SaveLoadSystem {

	private:

		//Core* core;
		void saveEntitiesRecursively(Transform* parent, rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entityNode, Core* core);
		void loadEntitiesRecursively(rapidxml::xml_node<>* parentNode, Entity* parent, Core* core);
		bool saveTransformComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Transform* transform);
		bool loadTransformComponent(Entity* ent, rapidxml::xml_node<>* entNode);
		bool saveMeshRendererComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, MeshRenderer* meshRenderer, Core* core);
		bool loadMeshRendererComponent(Entity* ent, rapidxml::xml_node<>* entNode, Core* core);

	public:

		SaveLoadSystem();
		void loadEntities(Core* core, std::string filePath);
		bool saveEntities(Core* core);
		//bool saveSceneCamera(SceneCamera* sceneCamera);
		//bool loadSceneCamera(SceneCamera* sceneCamera);
		

	};
}