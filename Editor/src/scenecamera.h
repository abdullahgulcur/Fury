#pragma once

#include "glm/glm.hpp"
#include "imgui/imgui.h" // bunlari yeri bence bura olmamali ; )
#include "glm/gtc/matrix_transform.hpp"
#include "glfwcontext.h"

namespace Fury {

	class Core;
}

using namespace Fury;

namespace Editor {

	class Editor;

	enum SceneCameraFlags {

		FirstCycle = 0,
		AllowMovement = 1,
		MouseTeleported = 2
	};

	class SceneCamera {

	private:

		float rotationSpeed = 0.02;
		float translationSpeed = 1000;
		float scrollSpeed = 50;
		float generalSpeed = 0.3f;

		UINT16 controlFlags;

		float lastX;
		float lastY;

	public:

		float nearClip = 0.1;
		float farClip = 1000000;
		int projectionType = 0;
		int fovAxis = 0;
		float fov;
		float aspectRatio = 1.77f;

		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;
		glm::mat4 projectionViewMatrix;

		glm::vec4 planes[6];

		unsigned int textureBuffer;
		unsigned int FBO;
		unsigned int RBO;

		float horizontalAngle = 0.f;
		float verticalAngle = 0.f;
		glm::vec3 position = glm::vec3(0, 0, -5);

		SceneCamera();
		void init();
		void update(float dt);
		void controlMouse(float dt);
		void teleportMouse(glm::vec2& mousePos, float& scenePosX, float& scenePosY, float& sceneRegionX,
			float& sceneRegionY, float& offset, bool& mouseTeleport);
		void frustum(glm::mat4& view_projection);
		void createFBO(int sizeX, int sizeY);
		void recreateFBO(int sizeX, int sizeY);
		void updateProjectionMatrix(int sizeX, int sizeY);
		bool intersectsAABB(glm::vec3 start, glm::vec3 end);
		bool load(std::string path);
		bool save(std::string path);
	};

}