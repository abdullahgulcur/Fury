#include "pch.h"
#include "particlesystem.h"
#include "core.h"
#include "entity.h"
#include "glewcontext.h"

namespace Fury {

	ParticleSystem::ParticleSystem(Entity* entity) : Component(entity) {
	}

	ParticleSystem::~ParticleSystem() {
	}

	void ParticleSystem::start() {

		ParticleSystem::initBillboardVAO();

		GlewContext* glew = Core::instance->glewContext;
		shaderProgramId = glew->loadShaders("C:/Projects/Fury/Core/src/shader/particle.vert", "C:/Projects/Fury/Core/src/shader/particle.frag");
	}

	void ParticleSystem::update(float dt) {

	}

	void ParticleSystem::onDraw(glm::mat4& pv, glm::vec3& pos) {

		GlewContext* glew = Core::instance->glewContext;
		glew->useProgram(shaderProgramId);
		glew->uniformMatrix4fv(glew->getUniformLocation(shaderProgramId, "PV"), 1, 0, &pv[0][0]);

		std::vector<glm::mat4> instanceArray;
		for (int i = 0; i < 100; i++) {

			float x = i % 10;
			float y = i / 10;

			glm::mat4 model(1);
			model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0));

			instanceArray.push_back(model);
		}

		unsigned int instanceBuffer;
		glew->genBuffers(1, &instanceBuffer);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->bufferData(0x8892, 100 * sizeof(glm::mat4), &instanceArray[0], 0x88E4);

		glew->bindVertexArray(billboardVAO);

		glew->enableVertexAttribArray(1);
		glew->vertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glew->enableVertexAttribArray(2);
		glew->vertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
		glew->enableVertexAttribArray(3);
		glew->vertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 2));
		glew->enableVertexAttribArray(4);
		glew->vertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 3));

		glew->vertexAttribDivisor(1, 1);
		glew->vertexAttribDivisor(2, 1);
		glew->vertexAttribDivisor(3, 1);
		glew->vertexAttribDivisor(4, 1);

		glew->drawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, instanceArray.size());

		glew->bindVertexArray(0);
		glew->deleteBuffers(1, &instanceBuffer);
	}

	void ParticleSystem::initBillboardVAO() {

		float billboardPlane[] = { 
			-0.1f, -0.1f,  0.f,
			-0.1f,  0.1f,  0.f,
			0.1f,  -0.1f,  0.f,
			0.1f,   0.1f,  0.f };

		unsigned int indices[] = { 0,1,2,1,3,2 };

		GlewContext* glew = Core::instance->glewContext;

		glew->genVertexArrays(1, &billboardVAO);
		glew->bindVertexArray(billboardVAO);

		unsigned int VBO;
		glew->genBuffers(1, &VBO);
		glew->bindBuffer(GL_ARRAY_BUFFER, VBO);
		glew->bufferData(GL_ARRAY_BUFFER, sizeof(billboardPlane), billboardPlane, GL_STATIC_DRAW);

		unsigned int EBO;
		glew->genBuffers(1, &EBO);
		glew->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glew->bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);

		//glEnableVertexAttribArray(1);
		//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

		//glEnableVertexAttribArray(2);
		//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

		glew->bindVertexArray(0);
	}

}