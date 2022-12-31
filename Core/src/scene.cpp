#include "pch.h"
#include "scene.h"
#include "core.h"

// todo: better variable names;

namespace Fury {

	Scene::Scene() {
		//name = "SampleScene";
		Scene::newEntity("Root");
	}

	Scene::Scene(std::string name) {
		this->name = name;
		Scene::newEntity("Root");
	}

	Scene::~Scene() {
		if (root)
			root->destroy();
	}

	//Scene::Scene(std::string name) {
	// find from file system and init scenegraph
	//	//name = "SampleScene";
	//	Scene::newEntity("Root");
	//}

	Entity* Scene::newEntity(std::string name) {
		root = new Entity(name, idCounter);
		entityIdToEntity.insert(std::pair<unsigned int, Entity*>(idCounter, root));
		idCounter++;
		return root;
	}

	Entity* Scene::newEntity(std::string name, Entity* parent) {
		Entity* ent = new Entity(name, parent, idCounter);
		entityIdToEntity.insert(std::pair<unsigned int, Entity*>(idCounter, ent));
		idCounter++;
		return ent;
	}

	Entity* Scene::newEntity(Entity* entity, Entity* parent) {
		Entity* cpy = new Entity(entity, parent, idCounter);
		entityIdToEntity.insert(std::pair<unsigned int, Entity*>(idCounter, cpy));
		idCounter++;
		return cpy;
	}

	Entity* Scene::duplicate(Entity* entity) {
		Entity* cpy = Scene::newEntity(entity, entity->transform->parent->entity);
		cloneRecursively(entity, cpy);
		return entity;
	}

	void Scene::cloneRecursively(Entity* copied, Entity* parent) {

		for (int i = 0; i < copied->transform->children.size(); i++) {

			Entity* entity = Scene::newEntity(copied->transform->children[i]->entity, parent);
			Scene::cloneRecursively(copied->transform->children[i]->entity, entity);
		}
	}

	void Scene::backup() {
		
		rootBackup = Scene::duplicate(root);
	}
}