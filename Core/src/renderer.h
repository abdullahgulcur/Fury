#pragma once
#include "glm/glm.hpp"

namespace Fury {

	class Core;
	class Entity;

	class Renderer {

	private:

	public:

		unsigned int defaultSphereVAO;
		unsigned int defaultSphereIndexCount;

		unsigned int framebufferProgramID;
		unsigned int quadVAO;

		unsigned int width;
		unsigned int height;

		/*Stats*/
		unsigned int drawCallCount;
		//unsigned int averageFPS;

		Renderer();
		void init();
		void initDefaultSphere();
		void update();
		//void drawMeshRendererRecursively(Entity* entity, glm::mat4& PV, glm::vec3& camPos);

		//void update();
	};
}