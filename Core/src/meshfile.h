#pragma once

#include "glm/glm.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "filesystem.h"

namespace Fury {

	class Core;

	struct __declspec(dllexport) Vertex {

		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
	};

	// dont forget to add reference !!!
	class __declspec(dllexport) MeshFile {

	private:

	public:

		unsigned int VAO;
		unsigned int indiceCount;

		float radius = -1;

		/* For editor */
		//unsigned int wireframeVAO;
		//unsigned int wireframeIndiceCount;

		unsigned int FBO;
		unsigned int RBO;
		unsigned int fileTextureId;

		MeshFile(File* file);
		~MeshFile();
		void loadModel(File* file);
		void processNode(File* file, aiNode* node, const aiScene* scene);
		void processMesh(File* file, aiMesh* mesh, const aiScene* scene);
		void createFBO(File* file);
	};
}