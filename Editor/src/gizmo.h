#pragma once

#include "glm/glm.hpp"

namespace Editor {

	class Editor;

	class Gizmo {

	private:

	public:

		unsigned int cubeVAO;
		unsigned int wireframeRendererProgramID;

		Gizmo();
		void update(const glm::mat4& MVP);
	};
}