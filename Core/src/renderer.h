#pragma once
#include "glm/glm.hpp"

namespace Fury {

	class Core;
	class Entity;

	struct CameraInfo {
		unsigned int FBO;
		glm::mat4 VP;
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec3 camPos;
		unsigned int width;
		unsigned int height;
		glm::vec4 planes[6];
	};

	class __declspec(dllexport) Renderer {

	private:

	public:

		unsigned int defaultSphereVAO;
		unsigned int defaultSphereIndexCount;

		unsigned int framebufferProgramID;
		unsigned int quadVAO;

		//unsigned int width;
		//unsigned int height;

		/*Stats*/
		unsigned int drawCallCount;
		//unsigned int averageFPS;

		unsigned int pickingProgramID;

		CameraInfo cameraInfo;

		Renderer();
		void init();
		void initDefaultSphere();
		void update(float dt);
		//void drawMeshRendererRecursively(Entity* entity, glm::mat4& PV, glm::vec3& camPos);
		Entity* detectAndGetEntityId(float mouseX, float mouseY, unsigned int FBO, unsigned int width, unsigned int height, glm::mat4& PV, glm::vec3& camPos, glm::vec4 planes[6]);
		//void update();
	};
}