#pragma once
#include "component/component.h"
#include "glewcontext.h"
#include "glm/glm.hpp"
#include "perlin/PerlinNoise.hpp"

#define RESOLUTION 16384
#define TILE_SIZE 256
#define MEM_TILE_ONE_SIDE 6

namespace Fury {

	class Core;

	struct TerrainVertexAttribs {

		glm::vec2 texturePos;
		glm::vec2 position;
		float scale;
		float texSize;
		float level;
		glm::mat4 model;
	};

	struct Vec2Int {
		int x;
		int z;
		Vec2Int(int _x, int _z) {
			x = _x; z = _z;
		}
	};

	class  __declspec(dllexport) Terrain : public Component {

	private:

		/* FOR TEXTURE STREAMING */

		// Each parcel is loaded according to camera's position.
		// There is a hieararchical loading mechanism, SSD -> RAM -> GPU.

	public:

		glm::vec2* blockPositions;
		glm::vec2* ringFixUpPositions;
		glm::vec2* interiorTrimPositions;
		glm::vec2* outerDegeneratePositions;
		float* rotAmounts;
		glm::vec2 smallSquarePosition;

		AABB_Box* blockAABBs;
		//AABB_Box* ringfixupAABBs;

		unsigned short clipmapResolution = 16;
		//unsigned short clipmapLevel = 4;
		float triangleSize = 1.f;

		unsigned int programID;
		unsigned int elevationMapTexture;
		unsigned int normalMapTexture;

		unsigned int diffuse;
		unsigned int normal;
		unsigned int metalness;
		unsigned int roughness;
		unsigned int ao;
		unsigned int displacement;

		std::vector<unsigned int> elevationMapTextures;
		std::vector<unsigned int> normalMapTextures;

		std::vector<unsigned int> blockIndices;
		unsigned int blockVAO;

		unsigned int ringFixUpVAO;
		std::vector<unsigned int> ringFixUpIndices;

		unsigned int smallSquareVAO;
		std::vector<unsigned int> smallSquareIndices;

		unsigned int outerDegenerateVAO;
		std::vector<unsigned int> outerDegenerateIndices;

		std::vector<unsigned int> interiorTrimIndices;
		unsigned int interiorTrimVAO;

		//unsigned int instanceBuffer;

		unsigned int perlinSeed = 48932;
		unsigned char perlinOctaves = 4;
		float perlinScale = 0.003f;
		float heightScale = 50.f;
		float perlinPersistence = 0.5f;

		/* about texture streaming */
		std::vector<Vec2Int> clipmapPositions;
		std::vector<Vec2Int> currentTileIndices;
		std::vector<Vec2Int> toroidalUpdateIndices;
		std::vector<Vec2Int> initialTexturePositions;

		

		unsigned char*** tiles; // 0: level, 1: tile, 2: byte
		//std::vector<std::string> heightmapNameList;

		unsigned char** lowDetailMipStack;
		
		Terrain();
		~Terrain();
		void init();
		void update();
		void calculateTerrainChunkPositions();
		void createHeightMap(std::vector<float>& heightImage);
		void createHeightMapWithPerlinNoise(std::vector<float>& heightImage);
		void createNormalMap(std::vector<char>& normalImage);
		void generateTerrainClipmapsVertexArrays();
		//float** getFlatHeightmap();
		char* getNormalMap(int size);
		void getMaxAndMinHeights(glm::vec2& bounds, const int& level, const glm::vec3& start, const glm::vec3& end);
		void setBoundariesOfClipmap(const int& level, glm::vec3& start, glm::vec3& end);
		void recalculateNormals(std::vector<float>& heightImage, std::vector<char>& normalImage);
		void streamFromTexture(int level, int oldCamPosX, int oldCamPosZ, int newCamPosX, int newCamPosZ);
		void updateStreamFrom_X(int level, int newCamPosX);
		void createDummyHeightmapTextureSet(int tileSize, int numTiles);
		void createHeightmap(const siv::PerlinNoise& perlin, int tileSize, int ind_x, int ind_z);
		void changeCurrentTileIndex_X(int level, int currentIndex);
		void loadAllHeightmapNames();
		Vec2Int getClipmapPosition(int level, glm::vec3& camPos);
		void loadAllHeightmaps();
		unsigned char* loadHeightmapFromDisk(int level, int x, int z);
		void sendHeightDataToGraphicsMemory(int level, unsigned char**& heightData);
		void updateHeightDataOnGraphicsMemory(int level, unsigned char*& heightData);
		void writeHeightDataToGPUBuffer(int level, unsigned char*& buffer, int x, int z);
		void writeHeightDataToMipStack(int level, unsigned char*& buffer, int x, int z, int divisionMultiplier);
		void createHeightMapTexture(unsigned char* heights);
		void createHeightMapTextureArray(unsigned char** heights);
		void recreateHeightMapTextureArray(unsigned char** heights);
		void recreateHeightMapTexture(unsigned char* heights);
		void setClipmapPositionHorizontal(int level, int x);
		void setClipmapPositionVertical(int level, int z);
		void applyPerlinNoiseOnPart(const siv::PerlinNoise& perlin, unsigned char* heights, int tileSize, int baseTileSize, int ind_x, int ind_z);
		void createDummyHeightmapTexture(int tileSize, int resolution);
		void createMipmapFromBase(unsigned char* baseTexture, unsigned char* newTexture, int baseSize);
		void createHeightmapImageFile(unsigned char* heights, int level, int newTileSize, int baseTileSize, int ind_x, int ind_z);
		void fucker(unsigned char* baseTexture, int baseSize);
		int getMaxMipLevel(int textureSize, int tileSize);
		void initTilesPtr();
		void deleteAllHeightData(unsigned char** heights);
		void deleteAllTiles();
		void loadMapFromDiscAsync(int level, int tileIndex, int x, int z);
		void updateHeightMapTextureArray(int level, unsigned char* heights);
		void updateHeightMapTextureArrayPartialVertical(int level, unsigned char* heights);
		void updateHeightMapTextureArrayPartialHorizontal(int level, unsigned char* heights);
		void loadTerrainChunkOnGraphicsMemoryVertical(int lodLevel, unsigned char*& heightData, int dir);
		void loadTerrainChunkOnGraphicsMemoryHorizontal(int lodLevel, unsigned char*& heightData, int dir);
		void writeHeightDataToGPUBufferVertical(int level, unsigned char* buffer, int z, int x);
		void writeHeightDataToGPUBufferHorizontal(int level, unsigned char* buffer, int z, int x);
		void loadAllLowDetailMipStack(int lodLevel);
		void deleteAllLowDetailMipStack();
		void deleteLowDetailMipStack(int lodLevel);
		void loadLowDetailMipStackAtLevel(int lodLevel);
		void getTerrainChunkBoundaries(int blockPosX, int blockPosZ, int level, glm::vec4& startInWorldSpace, glm::vec4& endInWorldSpace);
		glm::vec2 getMinAndMaxPointOfTerrainChunk(int startX, int startZ, int endX, int endZ, int level);
	};
}