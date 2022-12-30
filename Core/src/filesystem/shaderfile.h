#pragma once

namespace Fury {

	class Core;
	class File;

	class __declspec(dllexport) ShaderFile {

	private:

	public:

		//unsigned int programId;

		ShaderFile(std::string vertexShaderFilePath, std::string fragShaderFilePath, Core* core);
		ShaderFile(std::string vertexShaderFilePath, std::string fragShaderFilePath, std::string tesselationShaderFilePath, Core* core);
		//.....

		void load(std::string vertexShaderFilePath, std::string fragShaderFilePath, Core* core);
		void load(std::string vertexShaderFilePath, std::string fragShaderFilePath, std::string tesselationShaderFilePath, Core* core);

	};
}