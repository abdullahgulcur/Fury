#pragma once

#include "material/pbrmaterial.h"
#include "material/globalvolume.h"

namespace Fury {

	class ShaderFile;
	class TextureFile;
	class MaterialFile;
	class MeshFile;
	class SceneFile;
	class MeshRenderer;

	enum class FileType { folder, obj, mat, pmat, png, frag, vert, mp3, scene, undefined };

	class __declspec(dllexport) File {

	private:

	public:
		int id;
		File* parent;
		std::vector<File*> subfiles;
		std::string path;
		std::string name;
		FileType type;
		std::string extension; // get
		unsigned int textureID; // sonradan disari alinacak
	};

	//struct TextureFile : File {

	//};

	class Core;

	class __declspec(dllexport) FileSystem {

	private:


		int frameCounter = 0;
		int idCounter;
		//std::map<int, std::string> fileIdToPath;
		std::map<std::string, unsigned int> filePathToId;

		//Core* core;

	public:

		std::map<unsigned int, File*> files;

		File* currentOpenFile;
		File* rootFile;
		std::vector<File*> meshFiles;
		std::vector<File*> textureFiles;
		std::vector<File*> sceneFiles;
		std::vector<File*> matFiles;
		//std::vector<int> shdrFileIds;

		std::vector<File*> matFilesToBeAdded;

		std::map<TextureFile*, File*> texFileToFile;
		std::map<MaterialFile*, File*> matFileToFile;
		std::map<MeshFile*, File*> meshFileToFile;
		std::map<SceneFile*, File*> sceneFileToFile;

		std::map<File*, TextureFile*> fileToTexFile;
		std::map<File*, MaterialFile*> fileToMatFile;
		std::map<File*, MeshFile*> fileToMeshFile;
		std::map<File*, SceneFile*> fileToSceneFile;

		/*
		* ALGORITHMS
		* Sub Value Release or Component Delete: Find vector from map. Erase from vector. If vector empty then erase subvalue from map.
		* Sub Value Delete: Find the vector from map. Release each component's corresponding subvalue. Erase subvalue from map.
		* Sub Value Set: Find the vector and push.
		*/
		std::map<File*, std::vector<MeshRenderer*>> fileToMeshRendererComponents; // carry both meshfile and materialfile
		std::map<File*, std::vector<MaterialFile*>> fileToMaterials; // texture files for now


		//ShaderFile* defaultShaderFileWithTexture; // pbr, metallic setup
		//ShaderFile* pbrWithoutTexture; // pbr, metallic setup
		TextureFile* blackTexture;
		TextureFile* whiteTexture;
		TextureFile* flatNormalMapTexture;
		ShaderFile* pbrShader;
		//MaterialFile* pbrMaterialNoTexture;
		PBRMaterial* pbrMaterial;
		GlobalVolume* globalVolume;
		//MaterialFile* pbrMat;
		SceneFile* currentSceneFile;

		File* moveFileBuffer = NULL;

		/*
		* When file deleted, it is good too keep its parent in buffer
		* so that we would not encounter with null problem in editor.
		*/
		File* deletedFileParent = NULL;

		FileSystem();
		~FileSystem();
		void init();
		void onUpdate();
		void readDBFile(std::string fileName);
		void generateFileStructure(File* parent, std::filesystem::path path);
		void importAsset(File* file);
		void checkProjectFolder();
		void initMaterialFiles();
		bool hasSubFolder(File* file);
		bool subfolderCheck(File* fileToMove, File* fileToBeMoved);
		bool moveFile(File* toBeMoved, File* moveTo);
		bool subfolderAndItselfCheck(File* fileToMove, File* fileToBeMoved);
		void updateChildrenPathRecursively(File* file);
		void insertFileAlphabetically(File* file);
		void deleteFileFromTree(File* file);
		File* createPBRMaterialFile(File* parent, std::string fileName);
		File* newScene(File* parent, std::string fileName);
		void deleteFileCompletely(File* file);
		void getTreeIndices(File* file, std::vector<File*>& indices);
		unsigned int getSubFileIndex(File* file);
		File* duplicateFile(File* file);
		void rename(File* file, const char* newName);
		File* newFolder(File* parent, std::string fileName);
		void writeDBFile(std::string fileName);
		void checkExternalChanges(std::string path, bool& anyChange);
		void checkInternalChanges(File* file, bool& anyChange);
		FileType getFileType(std::string extension);
		void loadDefaultAssets();		
		File* createNewFile(File* parent, int id, std::filesystem::path path);
		void deleteFile(File* file);
		void deleteFileFromTree(File* parent, File* file);
		void importFiles(std::vector<std::string> files);

		std::string getProjectPathExternal();
		std::string getDocumentsPath();
		std::string getEnginePath();
		std::string getEngineName();
		std::string getProjectName();
		std::string getDBPath();
		std::string getAssetsPath();
		std::string getDBFolderName();
		std::string getAssetsFolderName();
		std::string getDBFileName();
		std::string getSceneManagerFileName();
		std::string getSceneCameraFileName();
		std::string getAssetsDBFileName();
		std::string getFileDBPath();
		std::string getAssetsFileDBPath();
		std::string getSceneManagerPath();
		std::string getSceneCameraPath();
		std::string getAvailableFileName(File* parent, std::string name);
		std::string str_tolower(std::string s) {
			std::transform(s.begin(), s.end(), s.begin(),
				[](unsigned char c) { return std::tolower(c); }
			);
			return s;
		}
		int iequals(const std::string a, const std::string b) {

			std::string a_lower = str_tolower(a);
			std::string b_lower = str_tolower(b);

			return std::strcmp(a_lower.c_str(), b_lower.c_str());
		}
	};
}