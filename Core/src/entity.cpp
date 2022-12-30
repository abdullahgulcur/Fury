#include "pch.h"
#include "entity.h"

namespace Fury {

	// for the root
	Entity::Entity(std::string name, unsigned int id) {
		this->id = id;
		this->name = name;
		transform = new Transform(this);
	}

	// when clicked new entity
	Entity::Entity(std::string name, Entity* parent, unsigned int id) {
		this->id = id;
		this->name = name;
		transform = new Transform(this, parent->transform);
	}

	// duplicate
	Entity::Entity(Entity* entity, Entity* parent, unsigned int id) {
		this->id = id;
		this->name = entity->name + "_cpy";
		transform = new Transform(this, entity->transform, parent->transform);

		for (auto& it : entity->components) {

			if (MeshRenderer* comp = dynamic_cast<MeshRenderer*>(it))
			{
				MeshRenderer* meshRendererComp = addComponent<MeshRenderer>();
				meshRendererComp->meshFile = comp->meshFile;
				meshRendererComp->materialFile = comp->materialFile;
			}
		}
	}

	Entity::~Entity() {
		
		for (Transform* transform : transform->children)
			delete transform->entity;

		for (Component* component : components)
			delete component;

		delete transform; // component destroy icinde olursa daha guzel gozukur ?
	}

	void Entity::rename(std::string name) {

		this->name = name;
	}

	bool Entity::hasChild(Entity* entity) {

		Transform* iter = entity->transform;

		while (iter->parent != NULL) {

			if (iter->parent == transform)
				return true;

			iter = iter->parent;
		}
		return false;
	}

	bool Entity::hasAnyChild() {

		if (transform->children.size() != 0)
			return true;

		return false;
	}

	bool Entity::attachEntity(Entity* entity) {

		if (entity->hasChild(this) || entity->transform->parent == transform)
			return false;

		entity->releaseFromParent();
		entity->transform->parent = transform;
		(transform->children).push_back(entity->transform);
		entity->transform->updateLocals();
		return true;
	}

	void Entity::releaseFromParent() {

		if (!transform->parent)
			return;

		for (int i = 0; i < transform->parent->children.size(); i++) {

			if (transform->parent->children[i] == transform) {
				transform->parent->children.erase(transform->parent->children.begin() + i);
				break;
			}
		}
	}

	////Entity* Entity::duplicate() {


	////	Entity* entity = new Entity(this, transform->parent->entity);
	////	cloneRecursively(this, entity);
	////	//entity->transform->updateSelfAndChild(); gerek var mi???
	////	return entity;
	////}

	//void Entity::cloneRecursively(Entity* copied, Entity* parent) {

	//	Entity* entity = new Entity(copied, parent);
	//	for (int i = 0; i < copied->transform->children.size(); i++)
	//		Entity::cloneRecursively(copied->transform->children[i]->entity, entity);
	//}

	////void Entity::cloneRecursively(Entity* copied, Entity* parent) {

	////	for (int i = 0; i < copied->transform->children.size(); i++) {

	////		Entity* entity = new Entity(copied->transform->children[i]->entity, parent);
	////		Entity::cloneRecursively(copied->transform->children[i]->entity, entity);
	////	}
	////}

	//void Entity::updateItselfAndChildrenTransforms()
	//{
	//	//Transform::setLocalTransformation();
	//	//Transform::setGlobalTransformation();
	//	//Transform::updatePhysics();

	//	//Transform::updateSelfAndChildRecursively();
	//}

	void Entity::destroy() {

		Entity::releaseFromParent();
		//Entity::destroyRecursively();
		delete this;
	}

	//void Entity::destroyEntity() {

	//	for (Component* component : components)
	//		component->destroy();

	//	delete transform; // component destroy icinde olursa daha guzel gozukur ?
	//	delete this;
	//}

	//void Entity::destroyRecursively() {

	//	//for (Transform* transform : transform->children)
	//	//	transform->entity->destroyRecursively();

	//	//transform->entity->destroyEntity();
	//}

}