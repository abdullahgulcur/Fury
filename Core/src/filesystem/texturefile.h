#pragma once

#include "filesystem.h"

#include "lodepng/lodepng.h"

namespace Fury {

	class __declspec(dllexport) TextureFile {

	private:

	public:

		unsigned int textureId;

		TextureFile(const char* imagepath);
		~TextureFile();
		unsigned int loadPNGTexture(const char* imagepath);
		static void decodeTextureFile(unsigned int& width, unsigned int& height, std::vector<unsigned char>& image, const char* imagepath);
		static void encodeTextureFile(unsigned int& width, unsigned int& height, std::vector<unsigned char>& image, const char* imagepath);
	};
}