#include "pch.h"

#include "filesystem.h"
#include "core.h"
#include "filesystem/materialfile.h"
#include "filesystem/shaderfile.h"
#include "filesystem/materialfile.h"
#include "filesystem/texturefile.h"
#include "filesystem/scenefile.h"
#include "filesystem/meshfile.h"
#include "component/meshrenderer.h"
#include "scene.h"

//TODO : 35. SATIR BUG OLABILIR. WRITEDBFILE FONKSIYONUINU DAHA DUZGUN KULLAN...

namespace Fury {

	FileSystem::FileSystem() {
	}

	FileSystem::~FileSystem() {

	}

	void FileSystem::init() {

		FileSystem::loadDefaultAssets();
		FileSystem::checkProjectFolder();
		currentOpenFile = rootFile;
		bool change = false;

		for (std::filesystem::path entry : std::filesystem::directory_iterator(rootFile->path))
			generateFileStructure(rootFile, entry);
		FileSystem::writeDBFile(FileSystem::getFileDBPath());
		/*
		*  If there is unnecessary entry in the filePathToId then it deletes that. BUG OLABILIR...
		*/
		for (auto it = filePathToId.cbegin(), next_it = it; it != filePathToId.cend(); it = next_it)
		{
			++next_it;
			auto fileIterator = files.find(it->second);
			if (fileIterator == files.end()) {
				filePathToId.erase(it);
				change = true;
			}
			else {
				if (fileIterator->second->path != it->first) {
					filePathToId.erase(it);
					change = true;
				}
			}
		}

		//FileSystem::checkExternalChanges(rootFile->path, change);
		//FileSystem::checkInternalChanges(rootFile, change); // gerek yok cunku eklerken dosyalara bakiyor, db ye bakmiyor basta.
		FileSystem::initMaterialFiles();

		if (change)
			FileSystem::writeDBFile(FileSystem::getFileDBPath());
	}

	void FileSystem::onUpdate() {

		if (frameCounter % 300 == 0) {

			bool change = false;
			FileSystem::checkExternalChanges(rootFile->path, change); 
			FileSystem::checkInternalChanges(rootFile, change);

			if (change) {
				FileSystem::writeDBFile(FileSystem::getFileDBPath()); // ?
				FileSystem::initMaterialFiles();
			}
			frameCounter = 0;
		}

		frameCounter++;
	}

	/*
	* If file is added then all corresponding assets will be imported to engine.
	*/
	void FileSystem::checkExternalChanges(std::string path, bool& anyChange) {

		for (std::filesystem::path entry : std::filesystem::directory_iterator(path)) {

			if (filePathToId.find(entry.string()) == filePathToId.end()) {
				// not found
				int parentFileId = filePathToId[path];
				File* parentFile = files[parentFileId];

				// if any moved
				if (moveFileBuffer && parentFile == moveFileBuffer->parent && entry.string() == moveFileBuffer->path) {

					if (parentFile->subfiles.size() == 0) {
						(parentFile->subfiles).push_back(moveFileBuffer);
						FileSystem::updateChildrenPathRecursively(moveFileBuffer);
						//FileSystem::changeAssetsKeyManually(toBeMoved, previousPath, destination);
					}
					else {
							FileSystem::insertFileAlphabetically(moveFileBuffer);
							FileSystem::updateChildrenPathRecursively(moveFileBuffer);
							//FileSystem::changeAssetsKeyManually(toBeMoved, previousPath, files[toBeMoved].path);
					}
					moveFileBuffer = NULL;
				}
				else { // new file
					FileSystem::generateFileStructure(parentFile, entry.string());

				}

				anyChange = true;

				//FileSystem::writeDBFile(FileSystem::getFileDBPath()); // ?
			}
			else {
				// found
				if(entry.extension().empty())
					FileSystem::checkExternalChanges(entry.string(), anyChange);
			}
		}
	}

	/*
	* If file is deleted then all corresponding assets will be deleted from engine.
	*/
	void FileSystem::checkInternalChanges(File* file, bool& anyChange) {

		//if (file == moveFileBuffer) {
		//	//moveFileBuffer = NULL;
		//	return;
		//}

		if (!std::filesystem::exists(file->path)) {

			deletedFileParent = file->parent;
			// delete it and its all children recursively
			FileSystem::deleteFileFromTree(file->parent, file);
			FileSystem::deleteFile(file);
			anyChange = true;

			//FileSystem::writeDBFile(FileSystem::getFileDBPath()); // ?
			return;
		}

		for (int i = 0; i < file->subfiles.size(); i++)
			FileSystem::checkInternalChanges(file->subfiles[i], anyChange);
	}

	void FileSystem::loadDefaultAssets() {

		// control these shaders, becasuse they fcked !!!
		//defaultShaderFileWithTexture = new ShaderFile("shader/PBR.vert", "shader/PBR.frag", core);
		//pbrShader = new ShaderFile("shader/PBR.vert", "shader/PBR.frag", core);
		//pbrWithoutTexture = new ShaderFile("C:/Projects/GameEngine2/Fury/Core/src/shader/PBRNoTexture.vert", 
			//"C:/Projects/GameEngine2/Fury/Core/src/shader/PBRNoTexture.frag", core);
			// 
		pbrMaterialNoTexture = new MaterialFile();
	}

	// bunun gubu uzun baska bir fonksiyon daha var. Import asset
	File* FileSystem::createNewFile(File* parent, int id, std::filesystem::path path) {

		File* subfile = new File;
		subfile->id = id;
		subfile->parent = parent;
		subfile->path = path.string();
		subfile->name = path.stem().string();
		subfile->extension = path.extension().string();
		subfile->type = FileSystem::getFileType(subfile->extension);
		//subfile->textureID = 0;
		(parent->subfiles).push_back(subfile);

		switch (subfile->type) {

		case FileType::folder: {
			//subfile->textureID = 0;
			break;
		}
		case FileType::obj: {
			MeshFile* meshFile = new MeshFile(subfile);
			meshFiles.push_back(subfile);
			meshFileToFile[meshFile] = subfile;
			fileToMeshFile[subfile] = meshFile;
			break;
		}	
		case FileType::mat: {
			if (std::filesystem::exists(subfile->path)) {
				matFilesToBeAdded.push_back(subfile);
				matFiles.push_back(subfile);
			}
			break;
		}
			
		//case FileType::pmat:
		//	//
		//	break;
		case FileType::png: {
			TextureFile* texFile = new TextureFile(subfile->path.c_str());
			subfile->textureID = texFile->textureId;
			textureFiles.push_back(subfile);
			texFileToFile[texFile] = subfile;
			fileToTexFile[subfile] = texFile;
			break;
		}
		case FileType::scene: {
			SceneFile* sceneFile = new SceneFile(subfile->path.c_str());
			sceneFiles.push_back(subfile);
			sceneFileToFile[sceneFile] = subfile;
			fileToSceneFile[subfile] = sceneFile;
			Core::instance->sceneManager->sceneFiles[subfile->name] = subfile;

			break;
		}
		//case FileType::frag:
		//	//
		//	break;
		//case FileType::vert:
		//	//
		//	break;
		//case FileType::mp3:
		//	//
		//	break;
		//case FileType::undefined:
		//	//
		//	break;
		//default:
			//subfile = new File;
			//break;
		}

		//File* subfile = NULL;
		//std::string filePath = path.string();
		//std::string extension = path.extension().string();
		//FileType type = FileSystem::getFileType(extension);

		//switch (type) {

		//case FileType::folder: {
		//	subfile = new File;
		//	break;
		//}
		//	
		//case FileType::obj: {
		//	MeshFile* file = new MeshFile(filePath, core);
		//	subfile = dynamic_cast<File*>(file);
		//	meshFileIds.push_back(id);
		//	break;
		//}
		//	
		//case FileType::mat: {
		//	MaterialFile* file = new MaterialFile(filePath, core);

		//	break;
		//}
		//	
		////case FileType::pmat:
		////	//
		////	break;
		////case FileType::png:
		////	//
		////	break;
		////case FileType::frag:
		////	//
		////	break;
		////case FileType::vert:
		////	//
		////	break;
		////case FileType::mp3:
		////	//
		////	break;
		////case FileType::undefined:
		////	//
		////	break;
		////default:
		//	//subfile = new File;
		//	//break;
		//}

		//if (subfile) {

		//	subfile->id = id;
		//	subfile->parent = parent;
		//	subfile->path = filePath;
		//	subfile->name = path.stem().string();
		//	subfile->extension = extension;
		//	subfile->type = type;
		//	(parent->subfiles).push_back(subfile);
		//}

		return subfile;
	}

	/*
	* Generates file structure tree but does not import files.
	* If file is not in DB then it is written into it;
	*/
	void FileSystem::generateFileStructure(File* parent, std::filesystem::path path) {

		//File* subfile = FileSystem::createNewFile(parent, idCounter, path);

		//fileIdToPath[idCounter] = subfile->path;
		//filePathToId[subfile->path] = idCounter;
		//files[idCounter] = subfile;
		//idCounter++;

		//if (subfile->type == FileType::folder) {

		//	for (std::filesystem::path entry : std::filesystem::directory_iterator(subfile->path)) {

		//		FileSystem::generateFileStructure(subfile, entry);
		//	}
		//}

		int fileId = -1;
		auto fileIterator = filePathToId.find(path.string());

		if (fileIterator != filePathToId.end())
			fileId = fileIterator->second;
		else {
			fileId = idCounter;
			filePathToId[path.string()] = fileId;
			idCounter++;

			// do not forget to update db file !
			//FileSystem::writeDBFile(FileSystem::getFileDBPath());
		}
		/*auto fileIterator = filePathToId.find(path.string());
		int fileId = fileIterator != filePathToId.end() ? fileIterator->second : fileId = idCounter++;*/

		File* subfile = new File;
		subfile->id = fileId;
		subfile->parent = parent;
		subfile->path = path.string();
		subfile->name = path.stem().string();
		subfile->extension = path.extension().string();
		subfile->type = FileSystem::getFileType(subfile->extension);
		//subfile->textureID = 0;
		(parent->subfiles).push_back(subfile);

		files[fileId] = subfile;

		FileSystem::importAsset(subfile);

		if (subfile->type == FileType::folder) {
			for (std::filesystem::path entry : std::filesystem::directory_iterator(subfile->path)) 
				FileSystem::generateFileStructure(subfile, entry);
		}
	}

	void FileSystem::importAsset(File* file) {

		switch (file->type) {

		case FileType::folder: {
			//subfile->textureID = 0;
			break;
		}
		case FileType::obj: {
			MeshFile* meshFile = new MeshFile(file);
			meshFiles.push_back(file);
			meshFileToFile[meshFile] = file;
			fileToMeshFile[file] = meshFile;
			break;
		}
		case FileType::mat: {
			if (std::filesystem::exists(file->path)) {
				matFilesToBeAdded.push_back(file);
				matFiles.push_back(file);
			}
			break;
		}

						  //case FileType::pmat:
						  //	//
						  //	break;
		case FileType::png: {
			TextureFile* texFile = new TextureFile(file->path.c_str());
			file->textureID = texFile->textureId;
			textureFiles.push_back(file);
			texFileToFile[texFile] = file;
			fileToTexFile[file] = texFile;
			break;
		}
		case FileType::scene: {
			SceneFile* sceneFile = new SceneFile(file->path.c_str());
			sceneFiles.push_back(file);
			sceneFileToFile[sceneFile] = file;
			fileToSceneFile[file] = sceneFile;
			Core::instance->sceneManager->sceneFiles[file->name] = file;
			break;
		}
							//case FileType::frag:
							//	//
							//	break;
							//case FileType::vert:
							//	//
							//	break;
							//case FileType::mp3:
							//	//
							//	break;
							//case FileType::undefined:
							//	//
							//	break;
							//default:
								//subfile = new File;
								//break;
		}
	}

	void FileSystem::initMaterialFiles() {

		for (int i = 0; i < matFilesToBeAdded.size(); i++) {

			File* file = matFilesToBeAdded[i];
			if (std::filesystem::exists(file->path)) {

				MaterialFile* matFile = new MaterialFile(file);
				matFileToFile[matFile] = file;
				fileToMatFile[file] = matFile;
			}
		}

		matFilesToBeAdded.clear();
	}

	void FileSystem::checkProjectFolder() {

		std::string projectPath = FileSystem::getProjectPathExternal();
		std::string dbPath = FileSystem::getDBPath();
		std::string assetsPath = FileSystem::getAssetsPath();
		std::string fileDBPath = FileSystem::getFileDBPath();
		std::string enginePath = FileSystem::getEnginePath();
		std::string sceneManagerPath = FileSystem::getSceneManagerPath();

		if (!std::filesystem::exists(enginePath))
			std::filesystem::create_directory(enginePath);

		if (!std::filesystem::exists(projectPath))
			std::filesystem::create_directory(projectPath);

		if (!std::filesystem::exists(dbPath))
			std::filesystem::create_directory(dbPath);

		if (!std::filesystem::exists(sceneManagerPath)) {
			
			rapidxml::xml_document<> doc;
			rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
			decl->append_attribute(doc.allocate_attribute("version", "1.0"));
			decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
			doc.append_node(decl);

			rapidxml::xml_node<>* activeSceneNode = doc.allocate_node(rapidxml::node_element, "ActiveScene");
			activeSceneNode->append_attribute(doc.allocate_attribute("Name", doc.allocate_string("null")));
			doc.append_node(activeSceneNode);

			rapidxml::xml_node<>* scenesNode = doc.allocate_node(rapidxml::node_element, "Scenes");
			doc.append_node(scenesNode);

			std::ofstream file_stored(sceneManagerPath);
			file_stored << doc;
			file_stored.close();
			doc.clear();
		}

		// Create root file
		idCounter = 0;
		filePathToId[assetsPath] = idCounter;
		rootFile = new File;
		rootFile->id = idCounter;
		rootFile->parent = NULL;
		rootFile->path = assetsPath;
		rootFile->name = FileSystem::getAssetsFolderName();
		rootFile->type = FileType::folder;
		files[idCounter] = rootFile;
		idCounter++;

		if (!std::filesystem::exists(assetsPath))
			std::filesystem::create_directory(assetsPath);

		if (std::filesystem::exists(fileDBPath))
			FileSystem::readDBFile(fileDBPath);
		else {

			rapidxml::xml_document<> doc;
			rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
			decl->append_attribute(doc.allocate_attribute("version", "1.0"));
			decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
			doc.append_node(decl);
			rapidxml::xml_node<>* filesNode = doc.allocate_node(rapidxml::node_element, "Files");
			doc.append_node(filesNode);
			std::ofstream file_stored(fileDBPath);
			file_stored << doc;
			file_stored.close();
			doc.clear();
		}
	}

	void FileSystem::readDBFile(std::string path) {

		std::ifstream file(path);

		if (file.fail())
			return;

		rapidxml::xml_document<> doc;
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');
		doc.parse<0>(&buffer[0]);

		rapidxml::xml_node<>* files_node = doc.first_node("Files");

		int max = -1;
		for (rapidxml::xml_node<>* fileNode = files_node->first_node("File"); fileNode; fileNode = fileNode->next_sibling()) {

			std::string filePath = fileNode->first_attribute("Path")->value();
			int id = atoi(fileNode->first_attribute("Id")->value());			
			filePathToId[filePath] = id;

			if (max < id)
				max = id;
		}
		idCounter = max + 1;
	
		file.close();
	}

	void FileSystem::writeDBFile(std::string path) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* filesNode = doc.allocate_node(rapidxml::node_element, "Files");
		doc.append_node(filesNode);

		for (auto& it : filePathToId) {
			rapidxml::xml_node<>* fileNode = doc.allocate_node(rapidxml::node_element, "File");
			fileNode->append_attribute(doc.allocate_attribute("Path", doc.allocate_string(it.first.c_str())));
			fileNode->append_attribute(doc.allocate_attribute("Id", doc.allocate_string(std::to_string(it.second).c_str())));
			filesNode->append_node(fileNode);
		}

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);
		std::ofstream file_stored(path);
		file_stored << doc;
		file_stored.close();
		doc.clear();
	}

	bool FileSystem::hasSubFolder(File* file) {

		for (int i = 0; i < file->subfiles.size(); i++) {

			if (file->subfiles[i]->type == FileType::folder)
				return true;
		}
		return false;
	}

	bool FileSystem::subfolderCheck(File* fileToMove, File* fileToBeMoved) {

		while (fileToMove->parent != NULL) {

			if (fileToMove->parent == fileToBeMoved)
				return true;

			fileToMove = fileToMove->parent;
		}
		return false;
	}

	bool FileSystem::subfolderAndItselfCheck(File* fileToMove, File* fileToBeMoved) {

		while (fileToMove->parent != NULL) {

			if (fileToMove == fileToBeMoved)
				return true;

			fileToMove = fileToMove->parent;
		}
		return false;
	}

	void FileSystem::updateChildrenPathRecursively(File* file) {

		std::stack<File*>fileStack;
		File* iter = file;

		while (iter != NULL) {

			fileStack.push(iter);
			iter = iter->parent;
		}

		std::string updatedPath = FileSystem::getProjectPathExternal() + "\\";

		while (fileStack.size() > 1) {

			File* popped = fileStack.top();
			fileStack.pop();
			updatedPath = updatedPath + popped->name + "\\";
		}

		std::string oldPath = file->path;
		File* popped = fileStack.top();
		fileStack.pop();
		updatedPath = updatedPath + popped->name + popped->extension;
		file->path = updatedPath;

		//fileIdToPath.erase(file->id);
		filePathToId.erase(oldPath);
		//fileIdToPath[file->id] = updatedPath;
		filePathToId[updatedPath] = file->id;
		//FileSystem::changeAssetsKeyManually(file->subfiles[i]->id, oldPath, files[file->subfiles[i]->id].path);

		for (int i = 0; i < file->subfiles.size(); i++) {

			FileSystem::updateChildrenPathRecursively(file->subfiles[i]);
		}

		//for (int i = 0; i < file->subfiles.size(); i++) {

		//	std::stack<File*>fileStack;
		//	File* iter = file->subfiles[i];

		//	while (iter != NULL) {

		//		fileStack.push(iter);
		//		iter = iter->parent;
		//	}

		//	std::string updatedPath = FileSystem::getProjectPathExternal() + "\\";

		//	while (fileStack.size() > 1) {

		//		File* popped = fileStack.top();
		//		fileStack.pop();
		//		updatedPath = updatedPath + popped->name + "\\";
		//	}

		//	std::string oldPath = file->subfiles[i]->path;
		//	File* popped = fileStack.top();
		//	fileStack.pop();
		//	updatedPath = updatedPath + popped->name + popped->extension;
		//	file->subfiles[i]->path = updatedPath;

		//	//FileSystem::changeAssetsKeyManually(file->subfiles[i]->id, oldPath, files[file->subfiles[i]->id].path);

		//	FileSystem::updateChildrenPathRecursively(file->subfiles[i]);
		//}
	}

	bool FileSystem::moveFile(File* toBeMoved, File* moveTo) {

		if (moveTo->type != FileType::folder || FileSystem::subfolderAndItselfCheck(moveTo, toBeMoved) || toBeMoved->parent == moveTo)
			return false;

		const auto copyOptions = std::filesystem::copy_options::recursive;
		std::string name = FileSystem::getAvailableFileName(moveTo, toBeMoved->name);
		std::filesystem::copy(toBeMoved->path, moveTo->path + "\\" + name + toBeMoved->extension, copyOptions);
		std::filesystem::remove_all(toBeMoved->path);

		FileSystem::deleteFileFromTree(toBeMoved);
		filePathToId.erase(toBeMoved->path);

		toBeMoved->name = name;
		toBeMoved->path = moveTo->path + "\\" + name + toBeMoved->extension;
		toBeMoved->parent = moveTo;
		moveFileBuffer = toBeMoved;
		/*moveFileBuffer = new File;
		moveFileBuffer->path = moveTo->path + "\\" + name + toBeMoved->extension;
		moveFileBuffer->parent = moveTo;*/
		//moveFileBuffer->name = name;
		return true;
		// 
		//if (moveTo->type != FileType::folder || FileSystem::subfolderAndItselfCheck(moveTo, toBeMoved) || toBeMoved->parent == moveTo)
		//	return;

		//std::string previousPath = toBeMoved->path;
		//File* oldParent = toBeMoved->parent;
		//toBeMoved->parent = moveTo;

		//const auto copyOptions = std::filesystem::copy_options::recursive;

		//if (moveTo->subfiles.size() == 0) {
		//	(moveTo->subfiles).push_back(toBeMoved);
		//	std::string destination = moveTo->path + "\\" + toBeMoved->name + toBeMoved->extension;
		//	toBeMoved->path = destination;
		//	FileSystem::updateChildrenPathRecursively(toBeMoved);
		//	std::filesystem::copy(previousPath, destination, copyOptions);
		//	std::filesystem::remove_all(previousPath);

		//	//FileSystem::changeAssetsKeyManually(toBeMoved, previousPath, destination);
		//}
		//else {

		//	toBeMoved->name = FileSystem::getAvailableFileName(toBeMoved, toBeMoved->name);
		//	toBeMoved->path = toBeMoved->parent->path + "\\" + toBeMoved->name + toBeMoved->extension;
		//	FileSystem::insertFileAlphabetically(toBeMoved);
		//	FileSystem::updateChildrenPathRecursively(toBeMoved);

		//	std::filesystem::copy(previousPath, toBeMoved->path, copyOptions);
		//	std::filesystem::remove_all(previousPath);

		//	//FileSystem::changeAssetsKeyManually(toBeMoved, previousPath, files[toBeMoved].path);
		//}

		//FileSystem::deleteFileFromTree(oldParent, toBeMoved);
	}

	//std::string FileSystem::getAvailableFileName(File* file, std::string name) {

	//	std::vector<int> indices;
	//	File* parent = NULL;
	//	bool flag = false;
	//	for (int i = 0; i < file->parent->subfiles.size(); i++) {

	//		if (FileSystem::iequals(name, file->parent->subfiles[i]->name) == 0)
	//			flag = true;

	//		std::string part = file->parent->subfiles[i]->name.substr(0, name.length());

	//		if (FileSystem::iequals(name, part) == 0) {

	//			parent = file->parent;
	//			indices.push_back(i);
	//		}
	//	}

	//	int max = 0;
	//	if (flag) {

	//		for (int i = 0; i < indices.size(); i++) {

	//			std::string temp = parent->subfiles[indices[i]]->name.substr(0, std::string(name).length());
	//			const char* firstPart = temp.c_str();

	//			std::string rightPart = parent->subfiles[indices[i]]->name.substr(std::string(name).length(), parent->subfiles[indices[i]]->name.length() - name.length());

	//			if (!rightPart.empty() && std::all_of(rightPart.begin(), rightPart.end(), ::isdigit)) {
	//				if (stoi(rightPart) > max)
	//					max = stoi(rightPart);
	//			}
	//		}

	//		return name + std::to_string(max + 1);
	//	}

	//	return name;
	//}

	std::string FileSystem::getAvailableFileName(File* parent, std::string name) {

		std::vector<int> indices;
		bool flag = false;
		for (int i = 0; i < parent->subfiles.size(); i++) {

			if (FileSystem::iequals(name, parent->subfiles[i]->name) == 0)
				flag = true;

			std::string part = parent->subfiles[i]->name.substr(0, name.length());

			if (FileSystem::iequals(name, part) == 0) {

				indices.push_back(i);
			}
		}

		int max = 0;
		if (flag) {

			for (int i = 0; i < indices.size(); i++) {

				std::string temp = parent->subfiles[indices[i]]->name.substr(0, std::string(name).length());
				const char* firstPart = temp.c_str();

				std::string rightPart = parent->subfiles[indices[i]]->name.substr(std::string(name).length(), parent->subfiles[indices[i]]->name.length() - name.length());

				if (!rightPart.empty() && std::all_of(rightPart.begin(), rightPart.end(), ::isdigit)) {
					if (stoi(rightPart) > max)
						max = stoi(rightPart);
				}
			}

			return name + std::to_string(max + 1);
		}

		return name;
	}

	void FileSystem::insertFileAlphabetically(File* file) {

		bool lastElementFlag = true;
		for (int i = 0; i < file->parent->subfiles.size(); i++) {

			if (FileSystem::iequals(file->name, file->parent->subfiles[i]->name) < 0) {

				file->parent->subfiles.insert(file->parent->subfiles.begin() + i, file);
				lastElementFlag = false;
				break;
			}
		}
		if (lastElementFlag)
			file->parent->subfiles.push_back(file);
	}

	// engine acilmadan once silindiyse hata verir !
	void FileSystem::deleteFile(File* file) {

		int fileId = file->id;
		std::string path = file->path;
		std::vector<File*> subfiles = file->subfiles;

		files.erase(fileId);
		//fileIdToPath.erase(fileId);
		filePathToId.erase(path);

		switch (file->type) {

		case FileType::folder: {
			break;
		}
		case FileType::obj: {
			//---------- destructor a konulabilir mi?
			/*std::vector<MeshRenderer*>& components = fileToMeshRendererComponents[file];
			for (auto& comp : components)
				comp->meshFile = NULL;
			fileToMeshRendererComponents.erase(file);*/
			//----------

			/*meshFiles.erase(std::remove(meshFiles.begin(), meshFiles.end(), file), meshFiles.end());
			MeshFile* meshFile = fileToMeshFile[file];
			fileToMeshFile.erase(file);
			meshFileToFile.erase(meshFile);*/
			MeshFile* meshFile = fileToMeshFile[file];
			delete meshFile;
			break;
		}
		case FileType::mat: {
			//----------
			//std::vector<MeshRenderer*>& components = fileToMeshRendererComponents[file];
			//for (auto& comp : components)
			//	comp->materialFile = NULL;// pbrMaterialNoTexture;
			//fileToMeshRendererComponents.erase(file);
			//----------

			//matFiles.erase(std::remove(matFiles.begin(), matFiles.end(), file), matFiles.end());
			MaterialFile* matFile = fileToMatFile[file];

		//	if (matFile) {
			//fileToMatFile.erase(file);
			//matFileToFile.erase(matFile);
			delete matFile;
		//	}
			break;
		}
		case FileType::png: {
			//----------
			//std::vector<MaterialFile*>& matFiles = fileToMaterials[file];
			//for (auto& matFile : matFiles)
			//	matFile->findTexFileAndRelease(file);
			//fileToMaterials.erase(file);
			//----------

			// destructorrr
			//textureFiles.erase(std::remove(textureFiles.begin(), textureFiles.end(), file), textureFiles.end());
			TextureFile* texFile = fileToTexFile[file];
			//fileToTexFile.erase(file);
			//texFileToFile.erase(texFile);
			delete texFile;
			break;
		}
		case FileType::scene: {
			//sceneFiles.erase(std::remove(sceneFiles.begin(), sceneFiles.end(), file), sceneFiles.end());
			SceneFile* sceneFile = fileToSceneFile[file];
			//fileToSceneFile.erase(file);
			//sceneFileToFile.erase(sceneFile);
			delete sceneFile;
			Core::instance->sceneManager->sceneFiles.erase(file->name);

			break;
		}
		}

		delete file;

		for (int i = 0; i < subfiles.size(); i++)
			FileSystem::deleteFile(subfiles[i]);
	}

	void FileSystem::deleteFileFromTree(File* file) {

		File* parent = file->parent;
		parent->subfiles.erase(std::remove(parent->subfiles.begin(), parent->subfiles.end(), file), parent->subfiles.end());
		//for (int i = 0; i < parent->subfiles.size(); i++) {

		//	if (parent->subfiles[i] == file) {

		//		parent->subfiles.erase(parent->subfiles.begin() + i);
		//		break;
		//	}
		//}
	}

	// kisasini yaz
	void FileSystem::deleteFileFromTree(File* parent, File* file) {

		for (int i = 0; i < parent->subfiles.size(); i++) {

			if (parent->subfiles[i] == file) {

				parent->subfiles.erase(parent->subfiles.begin() + i);
				break;
			}
		}
	}

	File* FileSystem::newFolder(File* parent, std::string fileName) {

		std::string name = FileSystem::getAvailableFileName(parent, fileName);
		std::filesystem::create_directory(parent->path + "\\" + name);

		//File* file = new File;
		//file->id = idCounter;
		//file->parent = parent;
		//file->path = parent->path + "\\" + fileName;
		//file->name = fileName;
		//file->extension = "";
		//file->type = FileType::folder;

		//if (parent->subfiles.size() == 0) {
		//	(parent->subfiles).push_back(file);
		//	std::filesystem::create_directory(file->path);
		//}
		//else {

		//	file->name = FileSystem::getAvailableFileName(file, file->name);
		//	file->path = parent->path + "\\" + file->name + file->extension;
		//	FileSystem::insertFileAlphabetically(file);

		//	std::filesystem::create_directory(file->path);
		//}

		//fileIdToPath[idCounter] = file->path;
		//filePathToId[file->path] = idCounter;
		//files[idCounter] = file;
		////loadFileToEngine(files[subFile->id]);
		//idCounter++;

		//std::string dbPath = FileSystem::getProjectPathExternal() + "\\Database";
		//std::string fileDBPath = dbPath + "\\fileDB.meta";
		//FileSystem::writeDBFile(fileDBPath);

		return parent;
	}

	/*
	* Creates pbr material file with extension mat.
	*/
	File* FileSystem::createPBRMaterialFile(File* parent, std::string fileName) {

		std::string name = FileSystem::getAvailableFileName(parent, fileName);
		std::string path = parent->path + "\\" + name + ".mat";
		
		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* materialNode = doc.allocate_node(rapidxml::node_element, "Material");
		rapidxml::xml_node<>* typeNode = doc.allocate_node(rapidxml::node_element, "Type");
		typeNode->append_attribute(doc.allocate_attribute("value", "PBR"));
		rapidxml::xml_node<>* texturesNode = doc.allocate_node(rapidxml::node_element, "Textures");
		materialNode->append_node(texturesNode);
		materialNode->append_node(typeNode);

		//rapidxml::xml_node<>* texturesNode = doc.allocate_node(rapidxml::node_element, "Textures");
		////for (int i = 0; i < 5; i++) {
		////	rapidxml::xml_node<>* texture = doc.allocate_node(rapidxml::node_element, "Texture");
		////	texture->append_attribute(doc.allocate_attribute("fileId", doc.allocate_string("-1")));
		////	texturesNode->append_node(texture);
		////}

		//for(int i = 0; )

		//materialNode->append_node(texturesNode);

		//for (rapidxml::xml_node<>* index_node = root_node->first_node("ActiveTextureIndices")->first_node("Index"); index_node; index_node = index_node->next_sibling()) {

		//	unsigned int textureIndex = atoi(index_node->first_attribute("value")->value());
		//	activeTextureIndices.push_back(textureIndex);
		//}

		doc.append_node(materialNode);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(path);
		file_stored << doc;
		file_stored.close();
		doc.clear();

		return parent;
	}

	File* FileSystem::newScene(File* parent, std::string fileName) {

		{
			std::string name = FileSystem::getAvailableFileName(parent, fileName);
			std::string path = parent->path + "\\" + name + ".scene";

			rapidxml::xml_document<> doc;
			rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
			decl->append_attribute(doc.allocate_attribute("version", "1.0"));
			decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
			doc.append_node(decl);

			rapidxml::xml_node<>* sceneNode = doc.allocate_node(rapidxml::node_element, "Scene");
			sceneNode->append_attribute(doc.allocate_attribute("Name", name.c_str()));
			doc.append_node(sceneNode);

			rapidxml::xml_node<>* entitiesNode = doc.allocate_node(rapidxml::node_element, "Entities");
			sceneNode->append_node(entitiesNode);

			std::string xml_as_string;
			rapidxml::print(std::back_inserter(xml_as_string), doc);

			std::ofstream file_stored(path);
			file_stored << doc;
			file_stored.close();
			doc.clear();

		}
		
		//struct SceneTemp {

		//	std::string name;
		//	float horizontalAngle;
		//	float verticalAngle;
		//	glm::vec3 position;
		//};

		//std::vector<SceneTemp> tempScenes;
		//std::string activeSceneName;

		//SceneTemp scnTmp;
		//scnTmp.name = fileName;
		//scnTmp.position.x = 0;
		//scnTmp.position.y = 0;
		//scnTmp.position.z = 0;
		//scnTmp.horizontalAngle = 0;
		//scnTmp.verticalAngle = 0;
		//tempScenes.push_back(scnTmp);

		////----------

		//{
		//	std::ifstream file(FileSystem::getSceneManagerPath());

		//	//if (file.fail())
		//	//	return false;

		//	rapidxml::xml_document<> doc;
		//	rapidxml::xml_node<>* active_scene_node = NULL;
		//	rapidxml::xml_node<>* scenes_node = NULL;

		//	std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		//	buffer.push_back('\0');

		//	doc.parse<0>(&buffer[0]);

		//	active_scene_node = doc.first_node("ActiveScene");
		//	scenes_node = doc.first_node("Scenes");
		//	activeSceneName = active_scene_node->first_attribute("Name")->value();

		//	for (rapidxml::xml_node<>* scene_node = scenes_node->first_node("Scenes"); scene_node; scene_node = scene_node->next_sibling()) {

		//		SceneTemp scnTmp;
		//		scnTmp.name = scene_node->first_attribute("Name")->value();
		//		rapidxml::xml_node<>* scene_camera_node = scene_node->first_node("SceneCamera");
		//		scnTmp.position.x = atof(scene_camera_node->first_node("Position")->first_attribute("X")->value());
		//		scnTmp.position.y = atof(scene_camera_node->first_node("Position")->first_attribute("Y")->value());
		//		scnTmp.position.z = atof(scene_camera_node->first_node("Position")->first_attribute("Z")->value());
		//		scnTmp.horizontalAngle = atof(scene_camera_node->first_node("Angle")->first_attribute("Horizontal")->value());
		//		scnTmp.verticalAngle = atof(scene_camera_node->first_node("Angle")->first_attribute("Vertical")->value());
		//		tempScenes.push_back(scnTmp);
		//	}
		//	file.close();
		//}

		////------------------

		//{

		//	std::ifstream file(FileSystem::getSceneManagerPath());

		//	//if (file.fail())
		//	//	return;

		//	rapidxml::xml_document<> doc;
		//	rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		//	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		//	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		//	doc.append_node(decl);

		//	rapidxml::xml_node<>* active_scene_node = doc.allocate_node(rapidxml::node_element, "ActiveScene");
		//	active_scene_node->append_attribute(doc.allocate_attribute("Name", doc.allocate_string(activeSceneName.c_str())));
		//	doc.append_node(active_scene_node);

		//	rapidxml::xml_node<>* scenes_node = doc.allocate_node(rapidxml::node_element, "Scenes");
		//	doc.append_node(scenes_node);

		//	for (auto scn : tempScenes) {

		//		rapidxml::xml_node<>* scene = doc.allocate_node(rapidxml::node_element, "Scene");
		//		rapidxml::xml_node<>* scene_camera = doc.allocate_node(rapidxml::node_element, "SceneCamera");
		//		
		//		rapidxml::xml_node<>* positionNode = doc.allocate_node(rapidxml::node_element, "Position");
		//		positionNode->append_attribute(doc.allocate_attribute("X", doc.allocate_string(std::to_string(scn.position.x).c_str())));
		//		positionNode->append_attribute(doc.allocate_attribute("Y", doc.allocate_string(std::to_string(scn.position.y).c_str())));
		//		positionNode->append_attribute(doc.allocate_attribute("Z", doc.allocate_string(std::to_string(scn.position.z).c_str())));
		//		scene_camera->append_node(positionNode);

		//		rapidxml::xml_node<>* angleNode = doc.allocate_node(rapidxml::node_element, "Angle");
		//		angleNode->append_attribute(doc.allocate_attribute("Horizontal", doc.allocate_string(std::to_string(scn.horizontalAngle).c_str())));
		//		angleNode->append_attribute(doc.allocate_attribute("Vertical", doc.allocate_string(std::to_string(scn.verticalAngle).c_str())));
		//		scene_camera->append_node(angleNode);

		//		scene->append_node(scene_camera);
		//		scenes_node->append_node(scene);
		//	}

		//	std::string xml_as_string;
		//	rapidxml::print(std::back_inserter(xml_as_string), doc);

		//	std::ofstream file_stored(FileSystem::getSceneManagerPath());
		//	file_stored << doc;
		//	file_stored.close();
		//	doc.clear();
		//}

		return parent;
	}

	void FileSystem::deleteFileCompletely(File* file) {

		std::filesystem::remove_all(file->path);

		//std::vector<File*> files;
		//FileSystem::getTreeIndices(file, files);
		////std::sort(indices.begin(), indices.end(), std::greater<int>());

		//File* parent = file->parent;
		//FileSystem::deleteFileFromTree(file);

		///*for (int i = 0; i < indices.size(); i++) {

		//	switch (files[indices[i]].type) {

		//	case FileType::object: {

		//		std::vector<MeshRenderer*> addrs = meshes[files[indices[i]].path].meshRendererCompAddrs;
		//		meshes.erase(files[indices[i]].path);

		//		for (auto& it : addrs)
		//			it->mesh = &meshes["Null"];

		//		editor->scene->saveEditorProperties();

		//		break;
		//	}
		//	case FileType::material: {

		//		std::vector<MeshRenderer*> addrs = materials[files[indices[i]].path].meshRendererCompAddrs;
		//		materials.erase(files[indices[i]].path);

		//		for (auto& it : addrs)
		//			it->mat = &materials["Default"];

		//		editor->scene->saveEditorProperties();

		//		break;
		//	}
		//	case FileType::physicmaterial: {

		//		std::vector<Collider*> addrs = physicmaterials[files[indices[i]].path].colliderCompAddrs;
		//		physicmaterials.erase(files[indices[i]].path);

		//		for (auto& it : addrs)
		//			it->pmat = &physicmaterials["Default"];

		//		editor->scene->saveEditorProperties();

		//		break;
		//	}
		//	case FileType::texture: {

		//		textures[files[indices[i]].path].deleteTexture();
		//		textures.erase(files[indices[i]].path);

		//		for (auto& mat_iter : materials) {

		//			int count = 0;
		//			for (auto& texFileAddr : mat_iter.second.textureUnitFileAddrs) {

		//				if (texFileAddr == files[indices[i]].addr) {
		//					texFileAddr = NULL;
		//					mat_iter.second.textureUnits[count] = textures["whitetexture"].textureID;
		//					FileSystem::writeMaterialFile(files[mat_iter.second.fileAddr->id].path, mat_iter.second);
		//				}
		//				count++;
		//			}
		//		}

		//		break;
		//	}
		//	case FileType::vertshader: {

		//		for (auto& mat_iter : materials) {

		//			if (mat_iter.second.vertShaderFileAddr == files[indices[i]].addr) {
		//				mat_iter.second.vertShaderFileAddr = NULL;
		//				FileSystem::writeMaterialFile(files[mat_iter.second.fileAddr->id].path, mat_iter.second);

		//				mat_iter.second.deleteProgram();
		//				mat_iter.second.compileShaders("source/shader/Default.vert",
		//					FileSystem::getFragShaderPath(mat_iter.second.fragShaderFileAddr),
		//					editor->scene->dirLightTransforms.size(), editor->scene->pointLightTransforms.size());
		//			}
		//		}

		//		vertShaders.erase(files[indices[i]].path);

		//		break;
		//	}
		//	case FileType::fragshader: {

		//		for (auto& mat_iter : materials) {

		//			if (mat_iter.second.fragShaderFileAddr == files[indices[i]].addr) {
		//				mat_iter.second.fragShaderFileAddr = NULL;
		//				mat_iter.second.textureUnitFileAddrs.clear();
		//				mat_iter.second.textureUnits.clear();
		//				mat_iter.second.floatUnits.clear();
		//				FileSystem::writeMaterialFile(files[mat_iter.second.fileAddr->id].path, mat_iter.second);

		//				mat_iter.second.deleteProgram();
		//				mat_iter.second.compileShaders(FileSystem::getVertShaderPath(mat_iter.second.vertShaderFileAddr),
		//					"source/shader/Default.frag",
		//					editor->scene->dirLightTransforms.size(), editor->scene->pointLightTransforms.size());
		//			}
		//		}

		//		fragShaders.erase(files[indices[i]].path);

		//		break;
		//	}
		//	}
		//}*/

		//for (int i = 0; i < files.size(); i++) {

		//	std::filesystem::remove(files[i]->path);
		//	delete files[i];
		//}
	}

	void FileSystem::getTreeIndices(File* file, std::vector<File*>& indices) {

		for (int i = 0; i < file->subfiles.size(); i++)
			getTreeIndices(file->subfiles[i], indices);

		indices.push_back(file);
	}

	// eksik
	File* FileSystem::duplicateFile(File* file) {

		const auto copyOptions = std::filesystem::copy_options::recursive;
		std::string name = FileSystem::getAvailableFileName(file->parent, file->name);
		std::filesystem::copy(file->path, file->parent->path + "\\" + name + file->extension, copyOptions);

		//File* newFile = new File;
		//newFile->parent = file->parent;
		//newFile->extension = file->extension;
		//newFile->type = file->type;
		//newFile->name = FileSystem::getAvailableFileName(newFile, newFile->name.c_str());
		//newFile->path = newFile->parent->path + "\\" + newFile->name + newFile->extension;
		//FileSystem::insertFileAlphabetically(newFile);
		//FileSystem::updateChildrenPathRecursively(newFile);

		//const auto copyOptions = std::filesystem::copy_options::recursive;
		//std::filesystem::copy(file->path, newFile->path, copyOptions);

		//if (newFile->type == FileType::folder) {

		//	//FileSystem::generateFileStructure(newFile);

		//	std::vector<File*> files;
		//	FileSystem::getTreeIndices(newFile, files);

		//	//loadFilesToEngine(indices);
		//}
		////else
		//	//loadFileToEngine(files[subFile->id]);

		return file;
	}

	void FileSystem::rename(File* file, const char* newName) {

		if (FileSystem::iequals(newName, file->name) == 0)
			return;

		std::string oldPath = file->path;
		filePathToId.erase(oldPath);
		file->name = FileSystem::getAvailableFileName(file->parent, newName);
		file->path = file->parent->path + "\\" + file->name + file->extension;
		//fileIdToPath[file->id] = file->path;
		filePathToId[file->path] = file->id;

		file->parent->subfiles.erase(file->parent->subfiles.begin() + FileSystem::getSubFileIndex(file));
		FileSystem::insertFileAlphabetically(file);
		std::filesystem::rename(oldPath, file->path);

		for (int i = 0; i < file->subfiles.size(); i++)
			FileSystem::updateChildrenPathRecursively(file->subfiles[i]);

		FileSystem::writeDBFile(FileSystem::getFileDBPath());

		if (file->type == FileType::scene) {
			SceneFile* sceneFile = fileToSceneFile[file];
			sceneFile->renameFile(file->path, file->name);
			Core::instance->sceneManager->renameCurrentScene(file->name);
			Core::instance->sceneManager->saveSceneManagerFile();
		}

		//FileSystem::changeAssetsKeyManually(id, oldPath, files[id].path);

		/*if (files[id].type == FileType::material)
			writeMaterialFile(files[id].path, materials[files[id].path]);
		else if (files[id].type == FileType::physicmaterial)
			writePhysicMaterialFile(files[id].path, physicmaterials[files[id].path]);*/

	}

	unsigned int FileSystem::getSubFileIndex(File* file) {

		for (int i = 0; i < file->parent->subfiles.size(); i++) {

			if (file == file->parent->subfiles[i])
				return i;
		}
	}

	void FileSystem::importFiles(std::vector<std::string> files) {

		for (int i = 0; i < files.size(); i++) {

			const auto copyOptions = std::filesystem::copy_options::recursive;
			std::filesystem::path fullPath = files[i];
			std::string name = fullPath.stem().string();
			std::string extension = fullPath.extension().string();
			std::string newName = FileSystem::getAvailableFileName(currentOpenFile, name);
			std::filesystem::copy(fullPath, currentOpenFile->path + "\\" + newName + extension, copyOptions);
		}
	}


	FileType FileSystem::getFileType(std::string extension) {

		if (extension.empty())
			return FileType::folder;
		else if (extension == ".obj")
			return FileType::obj;
		else if (extension == ".png")
			return FileType::png;
		else if (extension == ".mat")
			return FileType::mat;
		else if (extension == ".pmat")
			return FileType::pmat;
		else if (extension == ".frag")
			return FileType::frag;
		else if (extension == ".vert")
			return FileType::vert;
		else if (extension == ".mp3")
			return FileType::mp3;
		else if (extension == ".scene")
			return FileType::scene;
		else
			return FileType::undefined;
	}

	std::string FileSystem::getDocumentsPath() {

		PWSTR ppszPath; // variable to receive the path memory block pointer.
		HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &ppszPath);

		std::wstring documentsPath;
		if (SUCCEEDED(hr))
			documentsPath = ppszPath; // make a local copy of the path

		CoTaskMemFree(ppszPath);      // free up the path memory block
		std::string documents(documentsPath.begin(), documentsPath.end());
		return documents;
	}

	std::string FileSystem::getEnginePath() {

		return FileSystem::getDocumentsPath() + "\\" + FileSystem::getEngineName();
	}

	std::string FileSystem::getProjectPathExternal() {

		return FileSystem::getEnginePath() + "\\" + FileSystem::getProjectName();
	}

	std::string FileSystem::getDBPath() {

		return FileSystem::getProjectPathExternal() + "\\" + FileSystem::getDBFolderName();
	}

	std::string FileSystem::getAssetsPath() {

		return FileSystem::getProjectPathExternal() + "\\" + FileSystem::getAssetsFolderName();
	}

	std::string FileSystem::getFileDBPath() {

		return FileSystem::getDBPath() + "\\" + FileSystem::getDBFileName();
	}

	std::string FileSystem::getAssetsFileDBPath() {

		return FileSystem::getDBPath() + "\\" + FileSystem::getAssetsDBFileName();
	}

	std::string FileSystem::getSceneManagerPath() {

		return FileSystem::getDBPath() + "\\" + FileSystem::getSceneManagerFileName();
	}

	std::string FileSystem::getSceneCameraPath() {

		return FileSystem::getDBPath() + "\\" + FileSystem::getSceneCameraFileName();
	}

	std::string FileSystem::getDBFileName() {
		return "fileDB.meta";
	}

	std::string FileSystem::getAssetsDBFileName() {
		return "entity.xml";
	}

	std::string FileSystem::getSceneManagerFileName() {
		return "scenemanager.xml";
	}

	std::string FileSystem::getSceneCameraFileName() {
		return "scenecamera.xml";
	}

	std::string FileSystem::getAssetsFolderName() {
		return "Assets";
	}

	std::string FileSystem::getEngineName() {
		return "Fury";
	}

	std::string FileSystem::getProjectName() {
		return "Offroad";
	}

	std::string FileSystem::getDBFolderName() {
		return "Database";
	}

}