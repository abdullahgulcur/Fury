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

	struct __declspec(dllexport) AABB_Box {
		glm::vec4 start;
		glm::vec4 end;
	};

	// dont forget to add reference !!!
	class __declspec(dllexport) MeshFile {

	private:

	public:

		unsigned int VAO;
		unsigned int indiceCount;
		AABB_Box aabbBox;

		/* For editor */

		float radius = -1;
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