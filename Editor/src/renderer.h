#pragma once

#include "component/terrain.h"
#include "GL/glew.h"

using namespace Fury;

namespace Fury {
	class Entity;
	class GameCamera;
	class GlewContext;
}

namespace Editor {

	class Editor;
	class SceneCamera;

	class Renderer {

	private:

		int total = 0;
		int counter = 0;

		// some default shape VAO s
		unsigned int defaultSphereVAO;
		unsigned int defaultSphereIndexCount;

		unsigned int pickingProgramID;
		unsigned int framebufferProgramID;
		unsigned int quadVAO;

		unsigned int drawCallCount;

	public:

		Renderer();
		~Renderer();
		void init();
		void initDefaultSphere();
		void initMaterialFileTextures();
		void initMeshFileTextures();
		void update(float dt);
		void drawTerrain(SceneCamera* camera, GameCamera* gc, Terrain* terrain);
	/*	void drawInternalPart(int programID, int blockVAO, int ringFixupVAO, int smallSquareVAO, int blockIndiceCount,
			int ringFixupIndiceCount, int smallSquareIndiceCount, int clipmapResolution, 
			glm::vec3& camPos, glm::vec3& fakeDisplacement);*/
		void drawPartOfTerrain(GlewContext* glew, float x, float z, unsigned int programID, unsigned int indiceCount);
		//void drawMeshRendererRecursively(Entity* entity, glm::mat4& PV, glm::vec3& camPos);
		Entity* detectAndGetEntityId(float mouseX, float mouseY);
		void drawMeshRendererForPickingRecursively(Entity* entity);
	};
}