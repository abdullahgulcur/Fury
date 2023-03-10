#pragma once

#include "GL/glew.h"
#include "GLM/glm.hpp"

namespace Fury {

	class __declspec(dllexport) PBRMaterial {

	private:

	public:

		unsigned int pbrShaderProgramId_old;

		unsigned int pbrShaderProgramId;

		//unsigned int irradianceMap = 0;
		//unsigned int prefilterMap = 0;
		//unsigned int brdfLUTTexture = 0;


		//unsigned int quadVAO;

		// bunlar inherit edilebilir?
		//glm::vec3 lightPositions[4];
		//glm::vec3 lightColors[4];

		//----------------

		//unsigned int pbrShaderProgramId;
		//unsigned int equirectangularToCubemapShaderProgramId;
		//unsigned int irradianceShaderProgramId;
		//unsigned int prefilterShaderProgramId;
		//unsigned int brdfShaderProgramId;
		//unsigned int backgroundShaderProgramId;

		//unsigned int irradianceMap;
		//unsigned int prefilterMap;
		//unsigned int brdfLUTTexture;

		//unsigned int envCubemap;

		//unsigned int cubeVAO;
		//unsigned int quadVAO;

		//glm::vec3 lightPositions[4];
		//glm::vec3 lightColors[4];



		PBRMaterial();
		//void createCubeVAO();
		//void renderCube();
		//void createQuadVAO();
		//void renderQuad();
	};
}