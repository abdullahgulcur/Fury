#pragma once

#include "filesystem.h"

#include "lodepng/lodepng.h"

namespace Fury {

	enum class __declspec(dllexport) TextureType {
		WHITE, BLACK, NORMAL_MAP_FLAT, 
	};

	class __declspec(dllexport) TextureFile {

	private:

	public:

		unsigned int textureId;
		//int width;
		//int height;

		TextureFile(TextureType type);
		TextureFile(const char* imagepath);
		~TextureFile();
		unsigned int loadPNGTexture(const char* imagepath); // static ?
		static void decodeTextureFile(unsigned int& width, unsigned int& height, std::vector<unsigned char>& image, const char* imagepath);
		static void encodeTextureFile(unsigned int& width, unsigned int& height, std::vector<unsigned char>& image, const char* imagepath);
	};
}