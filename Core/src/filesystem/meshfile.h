#pragma once

#include "glm/glm.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "filesystem.h"

/* TODO */
/*
* For editor, variables that is used by editor would be added outside of the class, 
* It could be a map or something...  
*/

namespace Fury {

	struct __declspec(dllexport) Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
	};

	struct __declspec(dllexport) AABB_Box {
		glm::vec4 start;
		glm::vec4 end;
	};

	class __declspec(dllexport) MeshFile {

	private:

	public:

		unsigned int VAO;
		unsigned int indiceCount;
		AABB_Box aabbBox;

		/* For editor */
		unsigned int fileIconFBO;
		unsigned int fileIconRBO;
		unsigned int fileTextureId;

		MeshFile(File* file);
		~MeshFile();
		void loadModel(File* file);
		void processNode(File* file, aiNode* node, const aiScene* scene);
		void processMesh(File* file, aiMesh* mesh, const aiScene* scene);

#ifdef EDITOR_MODE
		void createFileIconFBO(File* file, float radius);
#endif

	};
}