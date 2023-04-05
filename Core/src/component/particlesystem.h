#pragma once

#include "component/component.h"
#include "glm/glm.hpp"

namespace Fury {

	struct __declspec(dllexport) Particle {

		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
		glm::mat4 model;
		unsigned int startTime;
		bool isAlive;
	};

	class __declspec(dllexport) ParticleSystem : public Component {

	private:

		unsigned int billboardVAO;
		unsigned int shaderProgramId;

	public:

		// inputs that is given via UI
		int maxParticleCount = 100;
		glm::vec3 direction;

		std::vector<Particle> particles;

		ParticleSystem();
		~ParticleSystem();
		void init();
		void initBillboardVAO();
		void onUpdate(float dt);
		void onDraw(glm::mat4& pv, glm::vec3& pos);
	};
}