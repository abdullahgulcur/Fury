#pragma once

#include "entity.h"
#include "component/gamecamera.h"

namespace Fury {

	class Core;

	class __declspec(dllexport) Scene {

	private:

	public:

		unsigned int idCounter = 0;
		std::string name;
		Entity* root;
		//Entity* rootBackup;
		std::map<unsigned int, Entity*> entityIdToEntity;
		GameCamera* primaryCamera = NULL;

		Scene();
		Scene(std::string name);
		~Scene();
		void start();
		void update(float dt);
		Entity* newEntity(std::string name);
		Entity* newEntity(std::string name, Entity* parent);
		Entity* newEntity(Entity* entity, Entity* parent);
		Entity* duplicate(Entity* entity);
		void cloneRecursively(Entity* copied, Entity* parent);
		void backup();
	};
}