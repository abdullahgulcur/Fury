#pragma once

#include "filesystem.h"
#include "filesystem/texturefile.h"
#include "filesystem/shaderfile.h"

#include "rapidXML/rapidxml_print.hpp"
#include "rapidXML/rapidxml.hpp"

namespace Fury {

	class Core;
	class MeshRenderer;

	class __declspec(dllexport) MaterialFile {

	private:

	public:

		unsigned int programId;
		std::vector<TextureFile*> textureFiles; // bir material e ayni texture lar konulmucak !
		//UINT8 activeTexture;
		std::vector<unsigned int> activeTextureIndices;

		/* For editor */
		unsigned int shaderTypeId;
		unsigned int FBO;
		unsigned int RBO;
		unsigned int fileTextureId;

		MaterialFile();
		~MaterialFile();
		MaterialFile(File* file);
		//void loadPBRShaderProgramWithoutTexture(File* file);
		void loadPBRShaderProgramWithTexture(File* file);
		void insertTexture(int textureIndex, File* texfile); // , File* file
		void addMeshRendererComponentDependency(MeshRenderer* component);
		void deleteMaterialFile();

		void replaceTexFile(int index);
		void releaseTexFile(int index);
		void releaseFile(File* file);
		void findTexFileAndRelease(File* file);

		/* For editor */
		void createFBO(File* file);
		void createFBO();
	};
}