#include "pch.h"
#include "meshrenderer.h"
#include "entity.h"
#include "filesystem/meshfile.h"
#include "filesystem/materialfile.h"
#include "core.h"

namespace Fury {

	MeshRenderer::MeshRenderer(Entity* entity) : Component(entity) {
	}

	MeshRenderer::~MeshRenderer() {

		MeshRenderer::releaseAllFiles();
	}

	void MeshRenderer::start() {

	}

	void MeshRenderer::update(float dt) {

		if (!meshFile || !materialFile)
			return;

		glm::mat4 model = entity->transform->model;
		glm::vec4 startInWorldSpace = model * meshFile->aabbBox.start;
		glm::vec4 endInWorldSpace = model * meshFile->aabbBox.end;

		//if (!camera->intersectsAABB(startInWorldSpace, endInWorldSpace))
		//    continue;

		PBRMaterial* pbrMaterial = Core::instance->fileSystem->pbrMaterial;
		GlewContext* glew = Core::instance->glewContext;
		GlobalVolume* globalVolume = Core::instance->fileSystem->globalVolume;

		CameraInfo& cameraInfo = Core::instance->renderer->cameraInfo;

		switch (materialFile->shaderType) {

		case ShaderType::PBR: {

			unsigned int pbrShaderProgramId = pbrMaterial->pbrShaderProgramId;
			glew->useProgram(pbrShaderProgramId);
			glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "projection"), 1, GL_FALSE, &cameraInfo.projection[0][0]);
			glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "view"), 1, GL_FALSE, &cameraInfo.view[0][0]);
			glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "model"), 1, GL_FALSE, &model[0][0]);
			glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, "camPos"), 1, &cameraInfo.camPos[0]);

			// bind pre-computed IBL data
			glew->activeTexture(GL_TEXTURE0);
			glew->bindTexture(GL_TEXTURE_CUBE_MAP, globalVolume->irradianceMap);
			glew->activeTexture(GL_TEXTURE1);
			glew->bindTexture(GL_TEXTURE_CUBE_MAP, globalVolume->prefilterMap);
			glew->activeTexture(GL_TEXTURE2);
			glew->bindTexture(GL_TEXTURE_2D, globalVolume->brdfLUTTexture);

			// illaki farkli channel texture lar bake lenmesine gerek yok, runtime da da halledilebilir. daha iyi use experience sunar
			for (int i = 0; i < materialFile->textureFiles.size(); i++) {

				std::string texStr = "texture" + std::to_string(i);
				glew->activeTexture(GL_TEXTURE3 + i);
				glew->bindTexture(GL_TEXTURE_2D, materialFile->textureFiles[i]->textureId);
			}

			glew->bindVertexArray(meshFile->VAO);
			glew->drawElements(0x0004, meshFile->indiceCount, 0x1405, (void*)0);
			glew->bindVertexArray(0);

			break;
		}
		case ShaderType::PBR_ALPHA: {
			break;
		}
		}
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