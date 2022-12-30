#include "pch.h"
#include "core.h"
#include "filesystem.h"
#include "shaderfile.h"

namespace Fury {
	
	ShaderFile::ShaderFile(std::string vertexShaderFilePath, std::string fragShaderFilePath, Core* core) {

		//load(vertexShaderFilePath, fragShaderFilePath, core);
		//programId = core->glewContext->loadPBRShaders(programId, vertexShaderFilePath.c_str(), fragShaderFilePath.c_str(), false, false, false, false, false);
		//programId = core->glewContext->loadShaders(vertexShaderFilePath.c_str(), fragShaderFilePath.c_str());
	}

	ShaderFile::ShaderFile(std::string vertexShaderFilePath, std::string fragShaderFilePath, std::string tesselationShaderFilePath, Core* core) {

	}

	void ShaderFile::load(std::string vertexShaderFilePath, std::string fragShaderFilePath, Core* core) {

		//programId = core->glewContext->loadShaders(vertexShaderFilePath.c_str(), fragShaderFilePath.c_str());
	}

	void ShaderFile::load(std::string vertexShaderFilePath, std::string fragShaderFilePath, std::string tesselationShaderFilePath, Core* core) {

	}
}