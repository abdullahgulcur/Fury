#pragma once
#include "component/component.h"
#include "glewcontext.h"
#include "glm/glm.hpp"

#define RESOLUTION 8192
#define TILE_SIZE 256
#define MEM_TILE_ONE_SIDE 4
#define MIP_STACK_DIVISOR_RATIO 8
#define MIP_STACK_SIZE (TILE_SIZE / MIP_STACK_DIVISOR_RATIO) * MEM_TILE_ONE_SIDE 


namespace Fury {

	/// <summary>
	///  TODO:
	///  View frustum culling
	///  faster file read for heightmaps
	///  merge functions in single func
	///  less data
	///  code review
	/// </summary>

	class Core;

	struct TerrainVertexAttribs {

		glm::vec2 position;
		glm::vec2 clipmapcenter;
		float level;
		glm::mat4 model;
	};

	class  __declspec(dllexport) Terrain : public Component {

	private:

		glm::vec3 cameraPosition;

	public:

		// is this part necessary ??
		glm::vec2* blockPositions;
		glm::vec2* ringFixUpPositions;
		glm::vec2* interiorTrimPositions;
		glm::vec2* outerDegeneratePositions;
		float* rotAmounts;
		glm::vec2 smallSquarePosition;

		AABB_Box* blockAABBs;
		//AABB_Box* ringfixupAABBs;

		unsigned short clipmapResolution = 120;
		//unsigned short clipmapLevel = 4;
		//float triangleSize = 1.f;

		unsigned int programID;
		unsigned int elevationMapTexture;

		unsigned int albedoTextureArray;
		unsigned int normalTextureArray;
		unsigned int maskTextureArray;
		//unsigned int roughnessTextureArray;
		//unsigned int aoTextureArray;
		//unsigned int heightTextureArray;

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

		unsigned char** heights;
		unsigned char** mipStack;

		float displacementMapScale = 1.5f;
		
		Terrain(Entity* entity);
		~Terrain();
		void start();
		void update(float dt);
		void onDraw(glm::mat4& pv, glm::vec3& pos);
		void drawElementsInstanced(GlewContext* glew, int size, unsigned int VAO, std::vector<TerrainVertexAttribs>& instanceArray, unsigned int indiceCount);
		void calculateBlockPositions(glm::vec3 camPosition, int level);
		AABB_Box getBoundingBoxOfClipmap(int clipmapIndex, int level);
		void generateTerrainClipmapsVertexArrays();
		void loadTerrainHeightmapOnInit(glm::vec3 camPos, int clipmapLevel);
		void loadHeightmapAtLevel(int level, glm::vec3 camPos, unsigned char* heightData);
		void streamTerrain(glm::vec3 newCamPos, int clipmapLevel);
		void streamTerrainHorizontal(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level);
		void streamTerrainVertical(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level);
		void loadFromDiscAndWriteGPUBufferAsync(int level, int texWidth, glm::ivec2 tileStart, glm::ivec2 border, unsigned char* heightData, glm::ivec2 toroidalUpdateBorder);
		void updateHeightMapTextureArrayPartial(int level, glm::ivec2 size, glm::ivec2 position, unsigned char* heights);
		void deleteHeightmapArray(unsigned char** heightmapArray);
		void createElevationMapTextureArray(unsigned char** heightmapArray);
		void writeHeightDataToGPUBuffer(glm::ivec2 index, int texWidth, unsigned char* heightMap, unsigned char* chunk, int level, glm::ivec2 toroidalUpdateBorder);
		unsigned char* loadTerrainChunkFromDisc(int level, glm::ivec2 index);
		void updateHeightsWithTerrainChunk(unsigned char* heights, unsigned char* chunk, glm::ivec2 pos, glm::ivec2 chunkSize, glm::ivec2 heightsSize);
		glm::ivec2 getClipmapPosition(int level, glm::vec3& camPos);
		glm::ivec2 getTileIndex(int level, glm::vec3& camPos);
		int getMaxMipLevel(int textureSize, int tileSize);
		unsigned char** createMipmaps(unsigned char* heights, int size);
		void createHeightmapImageFile(unsigned char* heights, int level, int newTileSize, int baseTileSize, int ind_x, int ind_z);
		void divideTerrainHeightmaps(unsigned char** heightMapList, int width);

		void createAlbedoMapTextureArray();
		void createNormalMapTextureArray();
		void createRoughnessMapTextureArray();
		void createAOMapTextureArray();
		void createHeightMapTextureArray();
	};
}