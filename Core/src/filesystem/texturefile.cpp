#include "pch.h"
#include "texturefile.h"
#include "core.h"
#include <chrono>
#include <iostream>
using namespace std::chrono;

namespace Fury {

	TextureFile::TextureFile(const char* imagepath) {


		textureId = TextureFile::loadPNGTexture(imagepath);

		
	}

	TextureFile::~TextureFile() {

		GlewContext* glewContext = Core::instance->glewContext;
		FileSystem* fileSystem = Core::instance->fileSystem;
		File* file = fileSystem->texFileToFile[this];

		std::vector<MaterialFile*>& matFiles = fileSystem->fileToMaterials[file];
		for (auto& matFile : matFiles)
			matFile->findTexFileAndRelease(file);
		fileSystem->fileToMaterials.erase(file);

		fileSystem->textureFiles.erase(std::remove(fileSystem->textureFiles.begin(), fileSystem->textureFiles.end(), file), fileSystem->textureFiles.end());
		fileSystem->fileToTexFile.erase(file);
		fileSystem->texFileToFile.erase(this);

		glewContext->deleteTextures(1, &textureId);
	}

	unsigned int TextureFile::loadPNGTexture(const char* imagepath) {

		unsigned width, height;
		std::vector<unsigned char> image;
		lodepng::decode(image, width, height, imagepath);

		unsigned int texture;

		// start
		auto start = high_resolution_clock::now(); 

		Core::instance->glewContext->generateTexture(texture, width, height, &image[0]);

		// end
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(stop - start);
		std::cout << "Time taken by function: " << duration.count() << " microseconds" << std::endl;

		return texture;
	}

	void TextureFile::decodeTextureFile(unsigned int& width, unsigned int& height, std::vector<unsigned char>& image, const char* imagepath) {

		lodepng::decode(image, width, height, imagepath);
	}

}