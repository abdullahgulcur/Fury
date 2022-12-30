#pragma once
#include "component/component.h"
#include "glewcontext.h"
#include <glm/glm.hpp>

namespace Fury {

	class Core;

	class  __declspec(dllexport) Terrain : public Component {

	private:

	public:

		unsigned short clipmapResolution = 32;
		//unsigned short clipmapResolution_temp = 32;
		unsigned short clipmapLevel = 10;
		float triangleSize = 10.f;

		unsigned int programID;
		unsigned int elevationMapTexture;
		unsigned int normalMapTexture;

		std::vector<unsigned int> blockIndices;
		unsigned int blockVAO;

		unsigned int ringFixUpHorizontalVAO;
		std::vector<unsigned int> ringFixUpHorizontalIndices;

		unsigned int ringFixUpVerticalVAO;
		std::vector<unsigned int> ringFixUpVerticalIndices;

		unsigned int smallSquareVAO;
		std::vector<unsigned int> smallSquareIndices;

		unsigned int outerDegenerateVAO;
		std::vector<unsigned int> outerDegenerateIndices;

		unsigned int heighmapTextureID;

		unsigned int elevationMapSize = 2048;

		float** heightMipMaps;
		char* normals;

		unsigned int perlinSeed = 6428;
		unsigned char perlinOctaves = 3;
		float perlinScale = 0.005f;
		float heightScale = 100.f;
		float perlinPersistence = 0.5f;

		Terrain();
		void init(Core* core);
		void createHeightMap(Core* core);
		void createNormalMap(Core* core);
		void generateTerrainClipmapsVertexArrays(Core* core);
		float** getFlatHeightmap();
		char* getNormalMap(int size);
		void getMaxAndMinHeights(glm::vec2& bounds, const int& level, const glm::vec3& start, const glm::vec3& end);
		void setBoundariesOfClipmap(const int& level, glm::vec3& start, glm::vec3& end);

	};
}