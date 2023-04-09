#include "pch.h"
#include "scene.h"
#include "component/particlesystem.h"
#include "component/terrain.h"
#include "core.h"

// todo: better variable names;

namespace Fury {

	Scene::Scene() {
		//name = "SampleScene";
		Scene::newEntity("Root");
	}

	Scene::Scene(std::string name) {
		//this->name = name;
		Scene::newEntity("Root");
	}

	Scene::~Scene() {
		if (root)
			root->destroy();
	}

	void Scene::start() {

	}

	void Scene::update(float dt) {

		std::stack<Entity*> entStack;
		entStack.push(Core::instance->sceneManager->currentScene->root);

		while (!entStack.empty()) {

			Entity* popped = entStack.top();
			entStack.pop();

			Terrain* terrain = popped->getComponent<Terrain>();
			if (terrain != NULL && primaryCamera != NULL)
				terrain->update(dt);

			ParticleSystem* particleSystem = popped->getComponent<ParticleSystem>();
			if (particleSystem != NULL)
				particleSystem->update(dt);

			for (Transform*& child : popped->transform->children)
				entStack.push(child->entity);
		}
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
		
		//rootBackup = Scene::duplicate(root);
	}
}