#pragma once

namespace Fury {

	class __declspec(dllexport) GlobalVolume {

	private:

	public:

		unsigned int equirectangularToCubemapShaderProgramId;
		unsigned int irradianceShaderProgramId;
		unsigned int prefilterShaderProgramId;
		unsigned int brdfShaderProgramId;
		unsigned int backgroundShaderProgramId;

		unsigned int cubeVAO;
		unsigned int quadVAO;
		unsigned int envCubemap;

		unsigned int irradianceMap = 0;
		unsigned int prefilterMap = 0;
		unsigned int brdfLUTTexture = 0;

		GlobalVolume();
		void createCubeVAO();
		void createQuadVAO();
		void renderCube();
		void renderQuad();

	};

}
