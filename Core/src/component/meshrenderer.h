#pragma once

#include "filesystem/meshfile.h"
#include "component/component.h"
#include "filesystem/materialfile.h"

namespace Fury {

	class Entity;
	class MeshFile;
	class MaterialFile;

	class __declspec(dllexport) MeshRenderer : public Component {

	private:

		void releaseFile(File* file);

	public:

		MeshFile* meshFile;
		MaterialFile* materialFile;

		MeshRenderer(Entity* entity);
		~MeshRenderer();
		void start();
		void update(float dt);
		void setMeshFileOnLoad(MeshFile* meshFile);
		void setMatFileOnLoad(MaterialFile* matFile);
		void setMeshFile(File* file, MeshFile* meshFile);
		void setMatFile(File* file, MaterialFile* matFile);
		void releaseMeshFile();
		void releaseMatFile();
		void releaseAllFiles();
	};
}