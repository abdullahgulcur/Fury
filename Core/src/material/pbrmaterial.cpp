#include "pch.h"
#include "pbrmaterial.h"
#include "glewcontext.h"
#include "core.h"

//#define STB_IMAGE_IMPLEMENTATION

namespace Fury {

	PBRMaterial::PBRMaterial() {

		GlewContext* glew = Core::instance->glewContext;
		pbrShaderProgramId_old = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/PBR.vert", "C:/Projects/Fury/Core/src/shader/PBR.frag");
		pbrShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/pbr/pbr.vert", "C:/Projects/Fury/Core/src/shader/pbr/pbr.frag");

		glUseProgram(pbrShaderProgramId);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "irradianceMap"), 0);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "prefilterMap"), 1);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "brdfLUT"), 2);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture0"), 3);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture1"), 4);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture2"), 5);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture3"), 6);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture4"), 7);
	}

	// ..mat()

	// .... mat()
	// butun materialler tek bir dosyada olali

}