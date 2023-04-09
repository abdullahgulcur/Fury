#pragma once

namespace Fury {

	class Entity;

	class __declspec(dllexport) Component {

	private:

	protected:

	public:
		Entity* entity;

		Component(Entity* entity);
		virtual ~Component();
		virtual void start(); // void* params, unsigned int length
		virtual void update(float dt);
		virtual void destroy();

	};
}