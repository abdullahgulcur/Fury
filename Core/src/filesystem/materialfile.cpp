#include "pch.h"
#include "materialfile.h"
#include "core.h"

namespace Fury {

	//// default material from no file
	//MaterialFile::MaterialFile() {

	//	shaderType = ShaderType::PBR_temp;
	//	programId = Core::instance->glewContext->loadShaders("C:/Projects/Fury/Core/src/shader/PBRNoTexture.vert",
	//		"C:/Projects/Fury/Core/src/shader/PBRNoTexture.frag");
	//	MaterialFile::createFBO();
	//}

	//MaterialFile::MaterialFile(ShaderType type) {

	//	FileSystem* fileSystem = Core::instance->fileSystem;

	//	shaderType = type;
	//	switch (type) {
	//	//case ShaderType::PBR_temp: {
	//	//	break;
	//	//}
	//	case ShaderType::PBR: {
	//		textureFiles.push_back(fileSystem->whiteTexture);
	//		textureFiles.push_back(fileSystem->flatNormalMapTexture);
	//		textureFiles.push_back(fileSystem->blackTexture);
	//		textureFiles.push_back(fileSystem->blackTexture);
	//		textureFiles.push_back(fileSystem->whiteTexture);
	//		break;
	//	}
	//	case ShaderType::PBR_ALPHA: {
	//		break;
	//	}
	//	}
	//}

	MaterialFile::~MaterialFile() {

		GlewContext* glewContext = Core::instance->glewContext;
		FileSystem* fileSystem = Core::instance->fileSystem;
		File* file = fileSystem->matFileToFile[this];

		MaterialFile::releaseAllTexFiles();

		std::vector<MeshRenderer*>& components = fileSystem->fileToMeshRendererComponents[file];
		for (auto& comp : components)
			comp->materialFile = NULL;
		fileSystem->fileToMeshRendererComponents.erase(file);

		glewContext->deleteRenderBuffers(1, &RBO);
		glewContext->deleteTextures(1, &fileTextureId);
		glewContext->deleteFrameBuffers(1, &FBO);

		fileSystem->matFiles.erase(std::remove(fileSystem->matFiles.begin(), fileSystem->matFiles.end(), file), fileSystem->matFiles.end());
		fileSystem->fileToMatFile.erase(file);
		fileSystem->matFileToFile.erase(this);
	}

	MaterialFile::MaterialFile(File* _file) {

		std::ifstream file(_file->path);

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* root_node = NULL;

		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');

		doc.parse<0>(&buffer[0]);

		root_node = doc.first_node("Material");
		std::string shaderType = root_node->first_node("Type")->first_attribute("value")->value();

		if (std::strcmp(&shaderType[0], "PBR") == 0) {

			this->shaderType = ShaderType::PBR;
			MaterialFile::initPBRMaterial(root_node, _file);
		}
		else if (std::strcmp(&shaderType[0], "PBR_Alpha") == 0) {

			this->shaderType = ShaderType::PBR_ALPHA;
		}

		MaterialFile::createFileIcon(_file);
	}

	void MaterialFile::initPBRMaterial(rapidxml::xml_node<>* root_node, File* file) {

		FileSystem* fileSystem = Core::instance->fileSystem;
		bool dirty = false;

		for (rapidxml::xml_node<>* texture_node = root_node->first_node("Textures")->first_node("Texture"); texture_node; texture_node = texture_node->next_sibling()) {

			unsigned int fileId = atoi(texture_node->first_attribute("fileId")->value());
			//unsigned int textureIndex = atoi(texture_node->first_attribute("index")->value());

			auto fileIterator = Core::instance->fileSystem->files.find(fileId);
			if (fileIterator == Core::instance->fileSystem->files.end()) { //  || fileIterator->second->type != FileType::png
				textureFiles.push_back(fileSystem->blackTexture);
				dirty = true;
				continue;
			}

			auto texFileIterator = Core::instance->fileSystem->fileToTexFile.find(fileIterator->second);
			if (texFileIterator == Core::instance->fileSystem->fileToTexFile.end()) {
				textureFiles.push_back(fileSystem->blackTexture);
				dirty = true;
				continue;
			}

			TextureFile* texFile = texFileIterator->second;
			textureFiles.push_back(texFile);
			//activeTextureIndices.push_back(textureIndex);

			// dependencies;
			Core::instance->fileSystem->fileToMaterials[fileIterator->second].push_back(this);
		}

		if (dirty)
			MaterialFile::savePBRMaterial(file);

		/*programId = Core::instance->glewContext->loadPBRShaders(programId, "C:/Projects/Fury/Core/src/shader/PBR.vert",
			"C:/Projects/Fury/Core/src/shader/PBR.frag", activeTextureIndices);*/
	}

	void MaterialFile::updatePBRMaterial(File* file) {

		MaterialFile::savePBRMaterial(file);

		this->shaderType = ShaderType::PBR;
		/*programId = Core::instance->glewContext->loadPBRShaders(programId, "C:/Projects/Fury/Core/src/shader/PBR.vert",
			"C:/Projects/Fury/Core/src/shader/PBR.frag", activeTextureIndices);*/
		MaterialFile::createFileIcon(file);
	}

	void MaterialFile::savePBRMaterial(File* file) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* materialNode = doc.allocate_node(rapidxml::node_element, "Material");
		rapidxml::xml_node<>* typeNode = doc.allocate_node(rapidxml::node_element, "Type");
		typeNode->append_attribute(doc.allocate_attribute("value", "PBR"));
		materialNode->append_node(typeNode);

		rapidxml::xml_node<>* texturesNode = doc.allocate_node(rapidxml::node_element, "Textures");

		for (int i = 0; i < textureFiles.size(); i++) {

			//File* file = Core::instance->fileSystem->texFileToFile[textureFiles[i]];
			rapidxml::xml_node<>* texture = doc.allocate_node(rapidxml::node_element, "Texture");

			auto fileIterator = Core::instance->fileSystem->texFileToFile.find(textureFiles[i]);
			if (fileIterator == Core::instance->fileSystem->texFileToFile.end()) //  || fileIterator->second->type != FileType::png
				texture->append_attribute(doc.allocate_attribute("fileId", doc.allocate_string(std::to_string(-1).c_str())));
			else
				texture->append_attribute(doc.allocate_attribute("fileId", doc.allocate_string(std::to_string(fileIterator->second->id).c_str())));
			//texture->append_attribute(doc.allocate_attribute("index", doc.allocate_string(std::to_string(activeTextureIndices[i]).c_str())));
			texturesNode->append_node(texture);
		}
		materialNode->append_node(texturesNode);
		doc.append_node(materialNode);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(file->path);
		file_stored << doc;
		file_stored.close();
		doc.clear();
	}

	void MaterialFile::insertTexture(int textureIndex, File* textureFile) {

		TextureFile* oldTexFile = textureFiles[textureIndex];

		TextureFile* texFile = Core::instance->fileSystem->fileToTexFile[textureFile];
		File* matFile = Core::instance->fileSystem->matFileToFile[this];

		auto fileIterator = Core::instance->fileSystem->texFileToFile.find(oldTexFile);

		if (fileIterator != Core::instance->fileSystem->texFileToFile.end()) {
			File* file = Core::instance->fileSystem->texFileToFile[oldTexFile];
			MaterialFile::releaseFile(file);
		}

		Core::instance->fileSystem->fileToMaterials[textureFile].push_back(this);
		textureFiles[textureIndex] = texFile;
		MaterialFile::updatePBRMaterial(matFile);

		/*if (fileIterator != Core::instance->fileSystem->texFileToFile.end() && fileIterator->second->type == FileType::png){
			File* file = Core::instance->fileSystem->texFileToFile[oldTexFile];
			MaterialFile::releaseFile(file);
		}

		Core::instance->fileSystem->fileToMaterials[textureFile].push_back(this);
		textureFiles[textureIndex] = texFile;
		MaterialFile::updatePBRMaterial(matFile);*/
		

		//for (int i = 0; i < activeTextureIndices.size(); i++) {

		//	if (textureIndex < activeTextureIndices[i]) {
		//		activeTextureIndices.insert(activeTextureIndices.begin() + i, textureIndex);
		//		textureFiles.insert(textureFiles.begin() + i, texFile);
		//		MaterialFile::updatePBRMaterial(matFile);

		//		//---- dependencies
		//		Core::instance->fileSystem->fileToMaterials[textureFile].push_back(this);
		//		return;
		//	}
		//	else if (activeTextureIndices[i] == textureIndex) {

		//		TextureFile* oldTexFile = textureFiles[i];
		//		textureFiles[i] = texFile;

		//		//---- dependencies
		//		File* file = Core::instance->fileSystem->texFileToFile[oldTexFile];
		//		MaterialFile::releaseFile(file);
		//		Core::instance->fileSystem->fileToMaterials[textureFile].push_back(this);
		//		MaterialFile::updatePBRMaterial(matFile);
		//		return;
		//	}
		//}

		//activeTextureIndices.push_back(textureIndex);
		//textureFiles.push_back(texFile);
		//MaterialFile::updatePBRMaterial(matFile);

		//---- dependencies
		Core::instance->fileSystem->fileToMaterials[textureFile].push_back(this);
	}

	void MaterialFile::findTexFileAndRelease(File* file) {

		FileSystem* fileSystem = Core::instance->fileSystem;

		TextureFile* texFile = Core::instance->fileSystem->fileToTexFile[file];

		for (int i = 0; i < textureFiles.size(); i++) {

			if (textureFiles[i] == texFile) {

				textureFiles[i] = fileSystem->blackTexture;
				//activeTextureIndices.erase(activeTextureIndices.begin() + i);
				//textureFiles.erase(textureFiles.begin() + i);

				// ne sikko bi kisim...
				File* f = Core::instance->fileSystem->matFileToFile[this];
				MaterialFile::updatePBRMaterial(f);
				break;
			}
		}
	}

	/*
	* Bu sekilde dogru da calisiyor olabilir iyice test et
	* Editor den bos secerse de bunu cagirabilir ? hayir, textureFiles ve activeTextureIndices den silmesi lazim. recompile etmesi de lazim...
	*/
	void MaterialFile::replaceTexFile(int index) { // 

		//TextureFile* texFile = textureFiles[index];
		//File* file = Core::instance->fileSystem->texFileToFile[texFile];
		//MaterialFile::releaseFile(file);
	}

	void MaterialFile::releaseFile(File* file) {

		std::vector<MaterialFile*>& matFiles = Core::instance->fileSystem->fileToMaterials[file];
		matFiles.erase(std::remove(matFiles.begin(), matFiles.end(), this), matFiles.end());
		if (matFiles.size() == 0) Core::instance->fileSystem->fileToMaterials.erase(file);
	}

	void MaterialFile::releaseTexFile(int index) {

		FileSystem* fileSystem = Core::instance->fileSystem;
		TextureFile* texFile = textureFiles[index];

		auto fileIterator = Core::instance->fileSystem->texFileToFile.find(texFile);
		if (fileIterator == Core::instance->fileSystem->texFileToFile.end()) //  || fileIterator->second->type != FileType::png
			return;

		//textureFiles.erase(textureFiles.begin() + index);
		//activeTextureIndices.erase(activeTextureIndices.begin() + index);
		File* file = fileIterator->second;
		MaterialFile::releaseFile(file);

		textureFiles[index] = fileSystem->blackTexture;

		// ne sikko bi kisim...
		file = Core::instance->fileSystem->matFileToFile[this];
		MaterialFile::updatePBRMaterial(file);
	}

	void MaterialFile::releaseAllTexFiles() {

		FileSystem* fileSystem = Core::instance->fileSystem;

		for (int i = 0; i < textureFiles.size(); i++) {

			File* file = Core::instance->fileSystem->texFileToFile[textureFiles[i]];
			MaterialFile::releaseFile(file);

			textureFiles[i] = fileSystem->blackTexture;
		}

		/*for (auto& texFile : textureFiles) {

			File* file = Core::instance->fileSystem->texFileToFile[texFile];
			MaterialFile::releaseFile(file);
		}*/
	}

	void MaterialFile::createFileIcon(File* file) {

		FileSystem* fileSystem = Core::instance->fileSystem;
		GlewContext* glewContext = Core::instance->glewContext;

		glm::vec3 camPos(0, 0, -2.1f);
		glm::mat4 view = glm::lookAt(camPos, camPos + glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
		glm::mat4 projection = glm::perspective(glm::radians(60.f), 1.f, 0.01f, 100.f);

		if (!FBO)// || FBO == Core::instance->fileSystem->pbrMaterialNoTexture->FBO
			Core::instance->glewContext->createFrameBuffer(FBO, RBO, fileTextureId, 64, 64);
		else
			Core::instance->glewContext->recreateFrameBuffer(FBO, fileTextureId, RBO, 64, 64);

		glewContext->bindFrameBuffer(FBO);
		glewContext->viewport(64, 64);
		glewContext->clearScreen(glm::vec3(0.1f, 0.1f, 0.1f));

		switch (shaderType) {
		case ShaderType::PBR: {

			unsigned int pbrShaderProgramId = fileSystem->pbrMaterial->pbrShaderProgramId_old;
			glewContext->useProgram(pbrShaderProgramId);
			glewContext->setVec3(pbrShaderProgramId, "camPos", camPos);
			glewContext->setMat4(pbrShaderProgramId, "PV", projection * view);
			glewContext->setMat4(pbrShaderProgramId, "model", glm::mat4(1));

			for (int i = 0; i < textureFiles.size(); i++) {
				glewContext->activeTex(i);
				glewContext->bindTex(textureFiles[i]->textureId);
				glewContext->setInt(pbrShaderProgramId, "texture" + std::to_string(i), i);
			}

			break;
		}
		case ShaderType::PBR_ALPHA: {
			break;
		}
		}

		glewContext->bindVertexArray(Core::instance->renderer->defaultSphereVAO);
		glewContext->drawElements(0x0005, Core::instance->renderer->defaultSphereIndexCount, 0x1405, (void*)0);
		glewContext->bindVertexArray(0);
		glewContext->bindFrameBuffer(0);

		file->textureID = fileTextureId;
	}

	//void MaterialFile::createFBO() {

	//	GlewContext* glew = Core::instance->glewContext;

	//	glew->genFramebuffers(1, &FBO);
	//	glew->bindFramebuffer(0x8D40, FBO);

	//	glew->genTextures(1, &fileTextureId);
	//	glew->bindTexture(0x0DE1, fileTextureId);
	//	glew->texImage2D(0x0DE1, 0, 0x1907, 64, 64, 0, 0x1907, 0x1401, NULL);
	//	glew->texParameteri(0x0DE1, 0x2801, 0x2601);
	//	glew->texParameteri(0x0DE1, 0x2800, 0x2601);

	//	glew->framebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, fileTextureId, 0);

	//	glew->genRenderbuffers(1, &RBO);
	//	glew->bindRenderbuffer(0x8D41, RBO);
	//	glew->renderbufferStorage(0x8D41, 0x88F0, 64, 64);
	//	glew->bindRenderbuffer(0x8D41, 0);

	//	glew->framebufferRenderbuffer(0x8D40, 0x821A, 0x8D41, RBO);

	//	if (glew->checkFramebufferStatus(0x8D40) != 0x8CD5)
	//		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	//	glew->bindFramebuffer(0x8D40, 0);

	//	glm::vec3 camPos(0, 0, -2.1f);
	//	glm::mat4 view = glm::lookAt(camPos, camPos + glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
	//	glm::mat4 projection = glm::perspective(glm::radians(60.f), 1.f, 0.01f, 100.f);

	//	glew->bindFrameBuffer(FBO);
	//	glew->viewport(64, 64);
	//	glew->clearScreen(glm::vec3(0.1f, 0.1f, 0.1f));

	//	glew->useProgram(programId);
	//	glew->setVec3(programId, "camPos", camPos);
	//	glew->setMat4(programId, "PV", projection * view);
	//	glew->setMat4(programId, "model", glm::mat4(1));

	//	glew->bindVertexArray(Core::instance->renderer->defaultSphereVAO);
	//	glew->drawElements_triStrip(Core::instance->renderer->defaultSphereIndexCount);
	//	glew->bindVertexArray(0);
	//	glew->bindFrameBuffer(0);
	//}

}