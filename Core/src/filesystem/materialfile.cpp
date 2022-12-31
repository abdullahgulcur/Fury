#include "pch.h"
#include "materialfile.h"
#include "core.h"

namespace Fury {

	// default material from no file
	MaterialFile::MaterialFile() {

		shaderTypeId = -1;
		programId = Core::instance->glewContext->loadShaders("C:/Projects/Fury/Core/src/shader/PBRNoTexture.vert",
			"C:/Projects/Fury/Core/src/shader/PBRNoTexture.frag");
		MaterialFile::createFBO();
	}

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
		const char* shaderType = root_node->first_node("Type")->first_attribute("value")->value();

		//if (std::strcmp(shaderType, "PBR") == 0) {

		//	shaderTypeId = 0;
		//	programId = Core::instance->fileSystem->pbrMaterialNoTexture->programId;
		//	fileTextureId = Core::instance->fileSystem->pbrMaterialNoTexture->fileTextureId;
		//	FBO = Core::instance->fileSystem->pbrMaterialNoTexture->FBO;
		//	RBO = Core::instance->fileSystem->pbrMaterialNoTexture->RBO;
		//	_file->textureID = fileTextureId;
		//}
		//else if (std::strcmp(shaderType, "PBR_With_Texture") == 0) {

		//	shaderTypeId = 1;

		//	for (rapidxml::xml_node<>* texture_node = root_node->first_node("Textures")->first_node("Texture"); texture_node; texture_node = texture_node->next_sibling()) {

		//		unsigned int fileId = atoi(texture_node->first_attribute("fileId")->value());
		//		unsigned int textureIndex = atoi(texture_node->first_attribute("index")->value());
		//		
		//		auto fileIterator = Core::instance->fileSystem->files.find(fileId);
		//		if (fileIterator != Core::instance->fileSystem->files.end()) {
		//			
		//			auto texFileIterator = Core::instance->fileSystem->fileToTexFile.find(fileIterator->second);
		//			if (texFileIterator != Core::instance->fileSystem->fileToTexFile.end()) {
		//				TextureFile* texFile = texFileIterator->second;
		//				textureFiles.push_back(texFile);
		//				activeTextureIndices.push_back(textureIndex);

		//				// dependencies;
		//				Core::instance->fileSystem->fileToMaterials[fileIterator->second].push_back(this);
		//			}
		//		}
		//	}
		//	programId = Core::instance->glewContext->loadPBRShaders(programId, "C:/Projects/GameEngine2/Fury/Core/src/shader/PBR.vert",
		//		"C:/Projects/GameEngine2/Fury/Core/src/shader/PBR.frag", activeTextureIndices);
		//	MaterialFile::createFBO(_file);
		//}

			if (std::strcmp(shaderType, "PBR") == 0) {

			shaderTypeId = 0;

			bool dirty = false;

			for (rapidxml::xml_node<>* texture_node = root_node->first_node("Textures")->first_node("Texture"); texture_node; texture_node = texture_node->next_sibling()) {

				unsigned int fileId = atoi(texture_node->first_attribute("fileId")->value());
				unsigned int textureIndex = atoi(texture_node->first_attribute("index")->value());

				auto fileIterator = Core::instance->fileSystem->files.find(fileId);
				if (fileIterator != Core::instance->fileSystem->files.end() && fileIterator->second->type == FileType::png) {

					auto texFileIterator = Core::instance->fileSystem->fileToTexFile.find(fileIterator->second);
					if (texFileIterator != Core::instance->fileSystem->fileToTexFile.end()) {
						TextureFile* texFile = texFileIterator->second;
						textureFiles.push_back(texFile);
						activeTextureIndices.push_back(textureIndex);

						// dependencies;
						Core::instance->fileSystem->fileToMaterials[fileIterator->second].push_back(this);
					}
				}
				else {
					dirty = true;
					break;
				}
			}

			if (dirty)
				MaterialFile::saveMaterialFile(_file);

			programId = Core::instance->glewContext->loadPBRShaders(programId, "C:/Projects/Fury/Core/src/shader/PBR.vert",
				"C:/Projects/Fury/Core/src/shader/PBR.frag", activeTextureIndices);
			MaterialFile::createFBO(_file);
		}
	}

	//void MaterialFile::loadPBRShaderProgramWithoutTexture(File* file) {

	//	rapidxml::xml_document<> doc;
	//	rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
	//	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	//	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	//	doc.append_node(decl);

	//	rapidxml::xml_node<>* materialNode = doc.allocate_node(rapidxml::node_element, "Material");
	//	rapidxml::xml_node<>* typeNode = doc.allocate_node(rapidxml::node_element, "Type");
	//	typeNode->append_attribute(doc.allocate_attribute("value", "PBR"));
	//	materialNode->append_node(typeNode);

	//	doc.append_node(materialNode);

	//	std::string xml_as_string;
	//	rapidxml::print(std::back_inserter(xml_as_string), doc);

	//	std::ofstream file_stored(file->path);
	//	file_stored << doc;
	//	file_stored.close();
	//	doc.clear();

	//	MaterialFile::deleteMaterialFile();

	//	shaderTypeId = 0;
	//	programId = Core::instance->fileSystem->pbrMaterialNoTexture->programId;
	//	fileTextureId = Core::instance->fileSystem->pbrMaterialNoTexture->fileTextureId;
	//	FBO = Core::instance->fileSystem->pbrMaterialNoTexture->FBO;
	//	RBO = Core::instance->fileSystem->pbrMaterialNoTexture->RBO;
	//	textureFiles.clear();
	//	activeTextureIndices.clear();

	//	file->textureID = fileTextureId;
	//}

	void MaterialFile::loadPBRShaderProgramWithTexture(File* _file) {

		// --- First, save the mat file ---

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

			File* file = Core::instance->fileSystem->texFileToFile[textureFiles[i]];
			rapidxml::xml_node<>* texture = doc.allocate_node(rapidxml::node_element, "Texture");
			texture->append_attribute(doc.allocate_attribute("fileId", doc.allocate_string(std::to_string(file->id).c_str())));
			texture->append_attribute(doc.allocate_attribute("index", doc.allocate_string(std::to_string(activeTextureIndices[i]).c_str())));
			texturesNode->append_node(texture);
		}
		materialNode->append_node(texturesNode);
		doc.append_node(materialNode);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(_file->path);
		file_stored << doc;
		file_stored.close();
		doc.clear();

		// --- Mat file is saved ---


		shaderTypeId = 0;
		programId = Core::instance->glewContext->loadPBRShaders(programId, "C:/Projects/Fury/Core/src/shader/PBR.vert",
			"C:/Projects/Fury/Core/src/shader/PBR.frag", activeTextureIndices);
		MaterialFile::createFBO(_file);
	}

	void MaterialFile::saveMaterialFile(File* file) {

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

			File* file = Core::instance->fileSystem->texFileToFile[textureFiles[i]];
			rapidxml::xml_node<>* texture = doc.allocate_node(rapidxml::node_element, "Texture");
			texture->append_attribute(doc.allocate_attribute("fileId", doc.allocate_string(std::to_string(file->id).c_str())));
			texture->append_attribute(doc.allocate_attribute("index", doc.allocate_string(std::to_string(activeTextureIndices[i]).c_str())));
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

	void MaterialFile::insertTexture(int textureIndex, File* textureFile) { //, File* file, en sondaki file kaldirilabilir

		TextureFile* texFile = Core::instance->fileSystem->fileToTexFile[textureFile];
		File* file = Core::instance->fileSystem->matFileToFile[this];
		//MaterialFile* matFile = Core::instance->fileSystem->fileToMatFile[file]; // matFile this i refer ediyor.

		if (activeTextureIndices.size() == 0) {
			activeTextureIndices.push_back(textureIndex);
			textureFiles.push_back(texFile);

			//---- dependencies
			Core::instance->fileSystem->fileToMaterials[textureFile].push_back(this);
		}
		else {

			if (std::count(activeTextureIndices.begin(), activeTextureIndices.end(), textureIndex) == 0) {

				for (int i = 0; i < activeTextureIndices.size(); i++) {

					if (textureIndex < activeTextureIndices[i]) {
						activeTextureIndices.insert(activeTextureIndices.begin() + i, textureIndex);
						textureFiles.insert(textureFiles.begin() + i, texFile);
						MaterialFile::loadPBRShaderProgramWithTexture(file);

						//---- dependencies
						Core::instance->fileSystem->fileToMaterials[textureFile].push_back(this);

						return;
					}
				}

				activeTextureIndices.push_back(textureIndex);
				textureFiles.push_back(texFile);

				//---- dependencies
				Core::instance->fileSystem->fileToMaterials[textureFile].push_back(this);

			}
			else {
				for (int i = 0; i < activeTextureIndices.size(); i++) {
					if (activeTextureIndices[i] == textureIndex) {

						//---- dependencies
						// release
						//MaterialFile::replaceTexFile(i);
						TextureFile* oldTexFile = textureFiles[i];
						File* file = Core::instance->fileSystem->texFileToFile[oldTexFile];
						MaterialFile::releaseFile(file);

						Core::instance->fileSystem->fileToMaterials[textureFile].push_back(this);
						textureFiles[i] = texFile;
					}
				}
			}
		}
		MaterialFile::loadPBRShaderProgramWithTexture(file);
	}

	void MaterialFile::findTexFileAndRelease(File* file) {

		TextureFile* texFile = Core::instance->fileSystem->fileToTexFile[file];

		for (int i = 0; i < activeTextureIndices.size(); i++) {

			if (textureFiles[i] == texFile) {

				activeTextureIndices.erase(activeTextureIndices.begin() + i);
				textureFiles.erase(textureFiles.begin() + i);

				// ne sikko bi kisim...
				File* f = Core::instance->fileSystem->matFileToFile[this];
				MaterialFile::loadPBRShaderProgramWithTexture(f);
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

		TextureFile* texFile = textureFiles[index];
		textureFiles.erase(textureFiles.begin() + index);
		activeTextureIndices.erase(activeTextureIndices.begin() + index);
		File* file = Core::instance->fileSystem->texFileToFile[texFile];
		MaterialFile::releaseFile(file);

		// ne sikko bi kisim...
		file = Core::instance->fileSystem->matFileToFile[this];
		MaterialFile::loadPBRShaderProgramWithTexture(file);
	}

	void MaterialFile::releaseAllTexFiles() {

		for (auto& texFile : textureFiles) {

			File* file = Core::instance->fileSystem->texFileToFile[texFile];
			MaterialFile::releaseFile(file);
		}
	}

	//void MaterialFile::load(std::string path, Core* core) {

	//	std::ifstream file(path);

	//	rapidxml::xml_document<> doc;
	//	rapidxml::xml_node<>* root_node = NULL;

	//	std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	//	buffer.push_back('\0');

	//	doc.parse<0>(&buffer[0]);

	//	root_node = doc.first_node("Material");
	//	rapidxml::xml_node<>* sampler2DsNode = root_node->first_node("Textures");
	//	std::map<unsigned int, File*>& files = Core::instance->fileSystem->files;

	//	//1 - Push all files to the parent class's texture files. 
	//	//2 - In the renderer, traverse all texture files and other properties of parent in an array
	//	//3 - In ui, do the same thing in step (1)

	//	//albedoTextureFile = dynamic_cast<TextureFile*>(files.find(atoi(sampler2DsNode->first_attribute("Albedo")->value()))->second);
	//	//normalTextureFile = dynamic_cast<TextureFile*>(files.find(atoi(sampler2DsNode->first_attribute("Normal")->value()))->second);
	//	//metallicTextureFile = dynamic_cast<TextureFile*>(files.find(atoi(sampler2DsNode->first_attribute("Metallic")->value()))->second);
	//	//roughnessTextureFile = dynamic_cast<TextureFile*>(files.find(atoi(sampler2DsNode->first_attribute("Roughness")->value()))->second);
	//	//aoTextureFile = dynamic_cast<TextureFile*>(files.find(atoi(sampler2DsNode->first_attribute("AmbientOcclusion")->value()))->second);

	//	for (rapidxml::xml_node<>* textureFileNode = sampler2DsNode->first_node("Texture"); textureFileNode; textureFileNode = textureFileNode->next_sibling()) {

	//		unsigned int fileId = atoi(textureFileNode->first_attribute("fileId")->value());

	//		if (fileId != -1) {

	//			File* file = files.find(fileId)->second;

	//			if (file) {
	//				TextureFile* textureFile = Core::instance->fileSystem->fileToTexFile.find(file)->second;
	//				textureFiles.push_back(textureFile);
	//			}
	//			else {

	//			}
	//		}
	//		else {
	//			
	//			TextureFile* textureFile = NULL;
	//			textureFiles.push_back(textureFile);
	//		}
	//		//TextureFile* file = dynamic_cast<TextureFile*>(files.find(atoi(textureFileNode->first_attribute("FileId")->value()))->second);
	//		//textureFiles.push_back(file);
	//	}

	//	//shaderFile = Core::instance->fileSystem->pbrShader;

	//	//bool fileMayCorrupted = false;

	//	//const char* vertFilePath = root_node->first_node("Shader")->first_attribute("Vert")->value();
	//	//const char* fragFilePath = root_node->first_node("Shader")->first_attribute("Frag")->value();
	//	//File* vertFileAddr = FileSystem::getVertShaderAddr(vertFilePath);
	//	//File* fragFileAddr = FileSystem::getFragShaderAddr(fragFilePath);

	//	//if (vertFileAddr == NULL) {

	//	//	vertFilePath = "source/shader/Default.vert";
	//	//	fileMayCorrupted = true;
	//	//}

	//	//if (fragFileAddr == NULL) {

	//	//	fragFilePath = "source/shader/Default.frag";
	//	//	fileMayCorrupted = true;
	//	//}

	//	//MaterialFile mat(filePtr, vertFileAddr, fragFileAddr, vertFilePath, fragFilePath, editor->scene->dirLightTransforms.size(), editor->scene->pointLightTransforms.size());

	//	//if (fragFileAddr != NULL) {

	//	//	for (rapidxml::xml_node<>* texpath_node = root_node->first_node("Sampler2Ds")->first_node("Texture"); texpath_node; texpath_node = texpath_node->next_sibling()) {

	//	//		File* textFile = FileSystem::getTextureFileAddr(texpath_node->first_attribute("Path")->value());
	//	//		mat.textureUnitFileAddrs.push_back(textFile);

	//	//		if (textFile != NULL)
	//	//			mat.textureUnits.push_back(textures[texpath_node->first_attribute("Path")->value()].textureID);
	//	//		else {

	//	//			mat.textureUnits.push_back(textures["whitetexture"].textureID);
	//	//			fileMayCorrupted = true;
	//	//		}
	//	//	}

	//	//	for (rapidxml::xml_node<>* texpath_node = root_node->first_node("Floats")->first_node("Float"); texpath_node; texpath_node = texpath_node->next_sibling())
	//	//		mat.floatUnits.push_back(atof(texpath_node->first_attribute("Value")->value()));
	//	//}

	//	//materials.insert({ path, mat });

	//	//if (fileMayCorrupted)
	//	//	FileSystem::writeMaterialFile(path, mat);
	//}

	void MaterialFile::createFBO(File* file) {

		if(!FBO)// || FBO == Core::instance->fileSystem->pbrMaterialNoTexture->FBO
			Core::instance->glewContext->createFrameBuffer(FBO, RBO, fileTextureId, 64, 64);
		else
			Core::instance->glewContext->recreateFrameBuffer(FBO, fileTextureId, RBO, 64, 64);

		glm::vec3 camPos(0, 0, -2.1f);
		glm::mat4 view = glm::lookAt(camPos, camPos + glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
		glm::mat4 projection = glm::perspective(glm::radians(60.f), 1.f, 0.01f, 100.f);

		GlewContext* glewContext = Core::instance->glewContext;
		glewContext->bindFrameBuffer(FBO);
		glewContext->viewport(64, 64);
		glewContext->clearScreen(glm::vec3(0.1f, 0.1f, 0.1f));

		glewContext->useProgram(programId);
		glewContext->setVec3(programId, "camPos", camPos);
		glewContext->setMat4(programId, "PV", projection * view);
		glewContext->setMat4(programId, "model", glm::mat4(1));

		for (int i = 0; i < activeTextureIndices.size(); i++) {
			glewContext->activeTex(i);
			glewContext->bindTex(textureFiles[i]->textureId);
			glewContext->setInt(programId, "texture" + std::to_string(activeTextureIndices[i]), i);
		}
		glewContext->bindVertexArray(Core::instance->renderer->defaultSphereVAO);
		glewContext->drawElements_triStrip(Core::instance->renderer->defaultSphereIndexCount);
		glewContext->bindVertexArray(0);
		glewContext->bindFrameBuffer(0);

		file->textureID = fileTextureId;
	}

	void MaterialFile::createFBO() {

		GlewContext* glew = Core::instance->glewContext;

		glew->genFramebuffers(1, &FBO);
		glew->bindFramebuffer(0x8D40, FBO);

		glew->genTextures(1, &fileTextureId);
		glew->bindTexture(0x0DE1, fileTextureId);
		glew->texImage2D(0x0DE1, 0, 0x1907, 64, 64, 0, 0x1907, 0x1401, NULL);
		glew->texParameteri(0x0DE1, 0x2801, 0x2601);
		glew->texParameteri(0x0DE1, 0x2800, 0x2601);

		glew->framebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, fileTextureId, 0);

		glew->genRenderbuffers(1, &RBO);
		glew->bindRenderbuffer(0x8D41, RBO);
		glew->renderbufferStorage(0x8D41, 0x88F0, 64, 64);
		glew->bindRenderbuffer(0x8D41, 0);

		glew->framebufferRenderbuffer(0x8D40, 0x821A, 0x8D41, RBO);

		if (glew->checkFramebufferStatus(0x8D40) != 0x8CD5)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		glew->bindFramebuffer(0x8D40, 0);

		glm::vec3 camPos(0, 0, -2.1f);
		glm::mat4 view = glm::lookAt(camPos, camPos + glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
		glm::mat4 projection = glm::perspective(glm::radians(60.f), 1.f, 0.01f, 100.f);

		glew->bindFrameBuffer(FBO);
		glew->viewport(64, 64);
		glew->clearScreen(glm::vec3(0.1f, 0.1f, 0.1f));

		glew->useProgram(programId);
		glew->setVec3(programId, "camPos", camPos);
		glew->setMat4(programId, "PV", projection * view);
		glew->setMat4(programId, "model", glm::mat4(1));

		glew->bindVertexArray(Core::instance->renderer->defaultSphereVAO);
		glew->drawElements_triStrip(Core::instance->renderer->defaultSphereIndexCount);
		glew->bindVertexArray(0);
		glew->bindFrameBuffer(0);
	}

}