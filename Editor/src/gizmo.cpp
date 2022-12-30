#include "pch.h"
#include "gizmo.h"
#include "editor.h"

namespace Editor {

	Gizmo::Gizmo() {

		std::vector<glm::vec3> points;
		points.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));
		points.push_back(glm::vec3(0.5f, -0.5f, -0.5f));
		points.push_back(glm::vec3(-0.5f, -0.5f, 0.5f));
		points.push_back(glm::vec3(0.5f, -0.5f, 0.5f));
		points.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));
		points.push_back(glm::vec3(-0.5f, -0.5f, 0.5f));
		points.push_back(glm::vec3(0.5f, -0.5f, -0.5f));
		points.push_back(glm::vec3(0.5f, -0.5f, 0.5f));

		points.push_back(glm::vec3(-0.5f, 0.5f, -0.5f));
		points.push_back(glm::vec3(0.5f, 0.5f, -0.5f));
		points.push_back(glm::vec3(-0.5f, 0.5f, 0.5f));
		points.push_back(glm::vec3(0.5f, 0.5f, 0.5f));
		points.push_back(glm::vec3(-0.5f, 0.5f, -0.5f));
		points.push_back(glm::vec3(-0.5f, 0.5f, 0.5f));
		points.push_back(glm::vec3(0.5f, 0.5f, -0.5f));
		points.push_back(glm::vec3(0.5f, 0.5f, 0.5f));

		points.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));
		points.push_back(glm::vec3(-0.5f, 0.5f, -0.5f));
		points.push_back(glm::vec3(0.5f, -0.5f, -0.5f));
		points.push_back(glm::vec3(0.5f, 0.5f, -0.5f));
		points.push_back(glm::vec3(-0.5f, -0.5f, 0.5f));
		points.push_back(glm::vec3(-0.5f, 0.5f, 0.5f));
		points.push_back(glm::vec3(0.5f, -0.5f, 0.5f));
		points.push_back(glm::vec3(0.5f, 0.5f, 0.5f));

		//lineRendererProgramID = Fury::loadShaders("src/shader/Line.vert", "src/shader/Line.frag");
		wireframeRendererProgramID = Core::instance->glewContext->loadShaders("C:/Projects/Fury/Core/src/shader/Line.vert", "C:/Projects/Fury/Core/src/shader/Line.frag");
		Core::instance->glewContext->initLineVAO(cubeVAO, points);
	}

	void Gizmo::update(const glm::mat4& MVP) {

		//Core::instance->glewContext->bindFrameBuffer(editor->sceneCamera->FBO);
		//Core::instance->glewContext->viewport(editor->menu->sceneRegion.x, editor->menu->sceneRegion.y);
		//Core::instance->glewContext->clearScreen();
		//Core::instance->glewContext->drawLineVAO(lineRendererProgramID, cubeVAO, MVP);
		//Core::instance->glewContext->bindFrameBuffer(0);
	}

}