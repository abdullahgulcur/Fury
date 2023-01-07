#pragma once

#include "component/terrain.h"

using namespace Fury;

namespace Fury {
	class Entity;
	class GameCamera;
}

namespace Editor {

	class Editor;
	class SceneCamera;

	class Renderer {

	private:

		// some default shape VAO s
		unsigned int defaultSphereVAO;
		unsigned int defaultSphereIndexCount;

		unsigned int pickingProgramID;
		unsigned int framebufferProgramID;
		unsigned int quadVAO;

		unsigned int drawCallCount;

	public:

		Renderer();
		void init();
		void initDefaultSphere();
		void initMaterialFileTextures();
		void initMeshFileTextures();
		void update();
		void drawTerrain(SceneCamera* camera, GameCamera* gc, Terrain* terrain);
		void drawMeshRendererRecursively(Entity* entity, glm::mat4& PV, glm::vec3& camPos);
		Entity* detectAndGetEntityId(float mouseX, float mouseY);
		void drawMeshRendererForPickingRecursively(Entity* entity);
	};
}