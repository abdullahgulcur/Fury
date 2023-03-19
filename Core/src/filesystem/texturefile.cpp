#include "pch.h"
#include "texturefile.h"
#include "core.h"
#include "FreeImage.h"

using namespace std::chrono;

namespace Fury {

	TextureFile::TextureFile(TextureType type) {

		GlewContext* glewContext = Core::instance->glewContext;

		switch (type) {
		case TextureType::BLACK: {
			int width = 1024;
			int height = 1024;
			int size = width * height * 4;
			unsigned char* image = new unsigned char[size];
			for (int i = 0; i < size; i += 4) {
				image[i] = (unsigned char)0;
				image[i + 1] = (unsigned char)0;
				image[i + 2] = (unsigned char)0;
				image[i + 3] = (unsigned char)255;
			}

			glewContext->generateTexture(textureId, width, height, image);
			delete[] image;
			break;
		}
		case TextureType::WHITE: {

			int width = 1024;
			int height = 1024;
			int size = width * height * 4;
			unsigned char* image = new unsigned char[size];
			for (int i = 0; i < size; i++)
				image[i] = (unsigned char)255;

			glewContext->generateTexture(textureId, width, height, image);
			delete[] image;
			break;
		}
		case TextureType::NORMAL_MAP_FLAT: {

			int width = 1024;
			int height = 1024;
			int size = width * height * 4;
			unsigned char* image = new unsigned char[size];
			for (int i = 0; i < size; i += 4) {
				image[i] = (unsigned char)127;
				image[i + 1] = (unsigned char)127;
				image[i + 2] = (unsigned char)255;
				image[i + 3] = (unsigned char)255;
			}

			glewContext->generateTexture(textureId, width, height, image);
			delete[] image;
			break;
		}
		}
	}

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

	// freeimage project is faster than lodepng...
	unsigned int TextureFile::loadPNGTexture(const char* imagepath) {

		auto start = high_resolution_clock::now();

		//unsigned width, height;
		//std::vector<unsigned char> image;
		//lodepng::decode(image, width, height, imagepath);

		unsigned int texture;

		// start

		FREE_IMAGE_FORMAT format = FreeImage_GetFileType(imagepath, 0);
		FIBITMAP* image = FreeImage_Load(format, imagepath);
		FIBITMAP* image32bits = FreeImage_ConvertTo32Bits(image);
		unsigned width = FreeImage_GetWidth(image32bits);
		unsigned height = FreeImage_GetHeight(image32bits);
		unsigned char* bits = (unsigned char*)FreeImage_GetBits(image32bits);
		unsigned char* bytes = new unsigned char[width * height * 4];

		for (int i = height-1; i >= 0; i--) {
			for (int j = 0; j < width; j++) {
				unsigned char b = bits[(i * width + j) * 4 + 0];
				unsigned char g = bits[(i * width + j) * 4 + 1];
				unsigned char r = bits[(i * width + j) * 4 + 2];

				bytes[((height - i - 1) * width + j) * 4 + 0] = r;
				bytes[((height - i - 1) * width + j) * 4 + 1] = g;
				bytes[((height - i - 1) * width + j) * 4 + 2] = b;
			}
		}

		//Core::instance->glewContext->generateTexture(texture, width, height, &image[0]);
		Core::instance->glewContext->generateTexture(texture, width, height, bytes);
		delete[] bytes;
		FreeImage_Unload(image);
		FreeImage_Unload(image32bits);

		// end
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(stop - start);
		std::cout << "Time taken by function: " << duration.count() << " microseconds" << std::endl;

		return texture;
	}

	void TextureFile::decodeTextureFile(unsigned int& width, unsigned int& height, std::vector<unsigned char>& image, const char* imagepath) {

		lodepng::decode(image, width, height, imagepath);
	}

	void TextureFile::encodeTextureFile(unsigned int& width, unsigned int& height, std::vector<unsigned char>& image, const char* imagepath) {

		lodepng::encode(imagepath, image, width, height, LodePNGColorType::LCT_GREY, 16);
	}

	void TextureFile::encodeTextureFile8Bits(unsigned int& width, unsigned int& height, std::vector<unsigned char>& image, const char* imagepath) {

		lodepng::encode(imagepath, image, width, height, LodePNGColorType::LCT_GREY, 8);
	}

}