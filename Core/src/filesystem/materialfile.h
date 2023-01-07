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
		void updatePBRMaterial(File* file);
		void savePBRMaterial(File* file);
		void insertTexture(int textureIndex, File* texfile); // , File* file

		void initPBRMaterial(rapidxml::xml_node<>* root_node, File* file);


		void replaceTexFile(int index);
		void releaseTexFile(int index);
		void releaseAllTexFiles();
		void releaseFile(File* file);
		void findTexFileAndRelease(File* file);

		/* For editor */
		void createFileIcon(File* file);
		void createFBO();
	};
}