#include "pch.h"
#include "meshrenderer.h"
#include "filesystem/meshfile.h"
#include "filesystem/materialfile.h"
#include "core.h"

namespace Fury {

	MeshRenderer::MeshRenderer() {
	}

	MeshRenderer::~MeshRenderer() {

		MeshRenderer::releaseAllFiles();
	}

	/*
	* Sets mmesh file on load time when scene is loaded. 
	*/
	void MeshRenderer::setMeshFileOnLoad(MeshFile* meshFile) {

		File* file = Core::instance->fileSystem->meshFileToFile[meshFile];
		Core::instance->fileSystem->fileToMeshRendererComponents[file].push_back(this);
		this->meshFile = meshFile;
	}

	/*
	* Sets material file on load time when scene is loaded.
	*/
	void MeshRenderer::setMatFileOnLoad(MaterialFile* matFile) {

		File* file = Core::instance->fileSystem->matFileToFile[matFile];
		Core::instance->fileSystem->fileToMeshRendererComponents[file].push_back(this);
		this->materialFile = matFile;
	}

	/*
	* Sets mesh file on run time when mesh file is changed by user in editor.
	*/
	void MeshRenderer::setMeshFile(File* file, MeshFile* meshFile) {

		if(this->meshFile)
			MeshRenderer::releaseMeshFile();

		//File* file = Core::instance->fileSystem->meshFileToFile[meshFile];
		Core::instance->fileSystem->fileToMeshRendererComponents[file].push_back(this);
		this->meshFile = meshFile;
	}

	/*
	* Sets material file on run time when material file is changed by user in editor.
	*/
	void MeshRenderer::setMatFile(File* file, MaterialFile* matFile) {

		//if(this->materialFile != Core::instance->fileSystem->pbrMaterialNoTexture)
		MeshRenderer::releaseMatFile();

		//File* file = Core::instance->fileSystem->matFileToFile[matFile];
		Core::instance->fileSystem->fileToMeshRendererComponents[file].push_back(this);
		this->materialFile = matFile;
	}

	/*
	* Bu sekilde dogru da calisiyor olabilir iyice test et
	*/
	void MeshRenderer::releaseMeshFile() {

		if (this->meshFile == NULL)
			return;

		File* file = Core::instance->fileSystem->meshFileToFile[meshFile];
		MeshRenderer::releaseFile(file);
		meshFile = NULL;
	}

	/*
	* Bu sekilde dogru da calisiyor olabilir iyice test et
	*/
	void MeshRenderer::releaseMatFile() {

		//if (this->materialFile == Core::instance->fileSystem->pbrMaterialNoTexture)
		//	return;

		File* file = Core::instance->fileSystem->matFileToFile[materialFile];
		MeshRenderer::releaseFile(file);
		materialFile = NULL;// Core::instance->fileSystem->pbrMaterialNoTexture;
	}

	void MeshRenderer::releaseFile(File* file) {

		std::vector<MeshRenderer*>& components = Core::instance->fileSystem->fileToMeshRendererComponents[file];
		components.erase(std::remove(components.begin(), components.end(), this), components.end());
		if (components.size() == 0) Core::instance->fileSystem->fileToMeshRendererComponents.erase(file);
	}

	void MeshRenderer::releaseAllFiles() {

		MeshRenderer::releaseMeshFile();
		MeshRenderer::releaseMatFile();
	}

}