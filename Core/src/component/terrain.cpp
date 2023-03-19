#include "pch.h"
#include "terrain.h"
#include "terrain.h"
#include "core.h"
#include "scenemanager.h"
#include "scene.h"
#include "glewcontext.h"
#include "lodepng/lodepng.h"
#include "FreeImage.h"

using namespace std::chrono;

namespace Fury {

	Terrain::Terrain() { }

	Terrain::~Terrain() {

		GlewContext* glew = Core::instance->glewContext;
		glew->deleteTextures(1, &elevationMapTexture);
		glew->deleteVertexArrays(1, &blockVAO);
		glew->deleteVertexArrays(1, &ringFixUpVAO);
		glew->deleteVertexArrays(1, &smallSquareVAO);
		glew->deleteVertexArrays(1, &outerDegenerateVAO);
		glew->deleteVertexArrays(1, &interiorTrimVAO);

		delete[] blockPositions;
		delete[] ringFixUpPositions;
		delete[] interiorTrimPositions;
		delete[] outerDegeneratePositions;
		delete[] rotAmounts;
		delete[] blockAABBs;

		Terrain::deleteHeightmapArray(heights);

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		for (int i = 0; i < lodLevel; i++)
			delete[] mipStack[i];

		delete[] mipStack;
	}

	void Terrain::init() {

		GameCamera* camera = Core::instance->sceneManager->currentScene->primaryCamera;
		GlewContext* glew = Core::instance->glewContext;
		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);

		blockPositions = new glm::vec2[12 * lodLevel + 4];
		ringFixUpPositions = new glm::vec2[4 * lodLevel + 4];
		interiorTrimPositions = new glm::vec2[lodLevel];
		outerDegeneratePositions = new glm::vec2[4 * lodLevel];
		rotAmounts = new float[lodLevel];
		blockAABBs = new AABB_Box[12 * lodLevel];

		heights = new unsigned char*[lodLevel];
		for (int i = 0; i < lodLevel; i++)
			heights[i] = new unsigned char[MEM_TILE_ONE_SIDE * MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * 2];

		mipStack = new unsigned char* [lodLevel];
		for (int i = 0; i < lodLevel; i++)
			mipStack[i] = new unsigned char[MIP_STACK_SIZE * MIP_STACK_SIZE];

		Terrain::generateTerrainClipmapsVertexArrays();

		programID = glew->loadShaders("C:/Projects/Fury/Core/src/shader/clipmap.vert", "C:/Projects/Fury/Core/src/shader/clipmap.frag");
		glew->useProgram(programID);
		glew->uniform1i(glew->getUniformLocation(programID, "irradianceMap"), 0);
		glew->uniform1i(glew->getUniformLocation(programID, "prefilterMap"), 1);
		glew->uniform1i(glew->getUniformLocation(programID, "brdfLUT"), 2);
		glew->uniform1i(glew->getUniformLocation(programID, "heightmapArray"), 3);

		cameraPosition = camera->position;
		Terrain::loadTerrainHeightmapOnInit(cameraPosition, lodLevel);
		Terrain::calculateBlockPositions(cameraPosition, lodLevel);

		for (int i = 0; i < lodLevel; i++)
			for (int j = 0; j < 12; j++)
				blockAABBs[12 * i + j] = Terrain::getBoundingBoxOfClipmap(j, i);
	}

	void Terrain::loadTerrainHeightmapOnInit(glm::vec3 camPos, int clipmapLevel) {

		for (int level = 0; level < clipmapLevel; level++)
			Terrain::loadHeightmapAtLevel(level, camPos, heights[level]);

		Terrain::createHeightMapTextureArray(heights);
	}

	void Terrain::loadHeightmapAtLevel(int level, glm::vec3 camPos, unsigned char* heightData) {

		glm::ivec2 tileIndex = Terrain::getTileIndex(level, camPos);
		glm::ivec2 tileStart = tileIndex - MEM_TILE_ONE_SIDE / 2;
		glm::ivec2 border = tileStart % MEM_TILE_ONE_SIDE;

		std::vector<std::thread> pool;

		for (int i = 0; i < MEM_TILE_ONE_SIDE; i++) {

			int startX = tileStart.x;

			for (int j = 0; j < MEM_TILE_ONE_SIDE; j++) {

				glm::ivec2 tileCoordinates(startX, tileStart.y);

				std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, TILE_SIZE * MEM_TILE_ONE_SIDE, glm::ivec2(startX, tileStart.y), border, heightData, border);
				pool.push_back(std::move(th));

				border.x++;
				border.x %= MEM_TILE_ONE_SIDE;
				startX++;
			}

			border.y++;
			border.y %= MEM_TILE_ONE_SIDE;
			tileStart.y++;
		}

		for (auto& it : pool)
			if (it.joinable())
				it.join();

		// DEBUG
		//std::vector<unsigned char> out(MIP_STACK_SIZE * MIP_STACK_SIZE);
		//for (int i = 0; i < MIP_STACK_SIZE * MIP_STACK_SIZE; i++)
		//	out[i] = mipStack[level][i];

		//unsigned int width = MIP_STACK_SIZE;
		//unsigned int height = MIP_STACK_SIZE;
		//std::string imagePath = "mipStack_" + std::to_string(level) + ".png";
		//TextureFile::encodeTextureFile8Bits(width, height, out, &imagePath[0]);
	}

	void Terrain::update() {

		int level = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		glm::vec3 camPosition = Core::instance->sceneManager->currentScene->primaryCamera->position;
		
		Terrain::calculateBlockPositions(camPosition, level);
		Terrain::streamTerrain(camPosition, level);
	}

	void Terrain::calculateBlockPositions(glm::vec3 camPosition, int level) {

		// Change this for debugging purposes--------
		//glm::vec3 camPos = camera->position;
		float fake = 1000000;
		glm::vec3 fakeDisplacement = glm::vec3(fake, 0, fake);
		glm::vec3 camPos = camPosition + fakeDisplacement;
		// '4' has to be constant, because every level has 4 block at each side.
		//int wholeClipmapRegionSize = clipmapResolution * 4 * (1 << level);
		// It has to be two. 
		int patchWidth = 2;
		int clipmapResolution = this->clipmapResolution;

		for (int i = 0; i < level; i++) {

			/*
			*         Z+
			*         ^
			*         |  This is our reference for numbers
			*         |
			* x+ <-----
			*/

			// Blocks move periodically according to camera's position.
			// For example:
			// Cam pos X       : 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ...
			// Block at level 0: 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10 ...
			// Block at level 1: 0, 0, 0, 0, 4, 4, 4, 4, 8, 8, 8  ...
			// Block at level 2: 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8  ...

			float requiredCameraDisplacement = patchWidth * (1 << i);
			float posX = (int)(camPos.x / requiredCameraDisplacement) * requiredCameraDisplacement;
			float posZ = (int)(camPos.z / requiredCameraDisplacement) * requiredCameraDisplacement;

			float patchTranslation = (1 << i) * clipmapResolution;
			float patchOffset = 1 << i;

			// For outer trim rotation. This '2' at the end is constant. Because of binary movement style
			int rotX = (int)(posX / requiredCameraDisplacement) % 2;
			int rotZ = (int)(posZ / requiredCameraDisplacement) % 2;

			// aciklama eklencek
			int parcelSizeInReal = TILE_SIZE * (1 << i);

			//// BLOCKS

			/*
			*  0 11 10  9
			*  1        8
			*  2        7
			*  3  4  5  6
			*/

			// 11. x, 1. z

			// 0
			glm::vec2 position(patchTranslation - fake + posX, patchTranslation - fake + posZ);
			position.x += patchOffset;
			position.y += patchOffset;
			blockPositions[i * 12 + 0] = position;

			// 1
			position.y += patchOffset - patchTranslation;
			blockPositions[i * 12 + 1] = position;
			position.y -= patchOffset;

			// 2
			position.y -= patchTranslation;
			blockPositions[i * 12 + 2] = position;
			position.y += patchOffset;

			// 3
			position.y -= patchTranslation;
			blockPositions[i * 12 + 3] = position;
			position.x += patchOffset;

			// 4
			position.x -= patchTranslation;
			blockPositions[i * 12 + 4] = position;

			// 5
			position.x -= patchTranslation + patchOffset;
			blockPositions[i * 12 + 5] = position;
			position.x += patchOffset;

			// 6
			position.x -= patchTranslation;
			blockPositions[i * 12 + 6] = position;
			position.y -= patchOffset;

			// 7
			position.y += patchTranslation;
			blockPositions[i * 12 + 7] = position;

			// 8
			position.y += patchTranslation + patchOffset;
			blockPositions[i * 12 + 8] = position;
			position.y -= patchOffset;

			// 9
			position.y += patchTranslation;
			blockPositions[i * 12 + 9] = position;

			// 10
			position.x += patchTranslation - patchOffset;
			blockPositions[i * 12 + 10] = position;
			position.x += patchOffset;

			// 11
			position.x += patchTranslation;
			blockPositions[i * 12 + 11] = position;

			// RING FIX-UP

			/*
			*    0
			*  1   3
			*    2
			*/

			// 0
			position = glm::vec2(-fake + posX, patchTranslation + patchOffset - fake + posZ);
			ringFixUpPositions[i * 4 + 0] = position;

			// 2
			position = glm::vec2(-fake + posX, (patchOffset - patchTranslation) * 2 - fake + posZ);
			ringFixUpPositions[i * 4 + 2] = position;

			// 1
			position = glm::vec2(patchTranslation + patchOffset - fake + posX, patchOffset * 2 - fake + posZ);
			ringFixUpPositions[i * 4 + 1] = position;

			// 3
			position = glm::vec2((patchOffset - patchTranslation) * 2 - fake + posX, patchOffset * 2 - fake + posZ);
			ringFixUpPositions[i * 4 + 3] = position;

			// INTERIOR TRIM

			position = glm::vec2(patchOffset * 2 * (1 - rotX) - fake + posX, patchOffset * 2 * (1 - rotZ) - fake + posZ);
			interiorTrimPositions[i] = position;

			if (rotX == 0 && rotZ == 0)
				rotAmounts[i] = 0.f;
			if (rotX == 0 && rotZ == 1)
				rotAmounts[i] = 90.f;
			if (rotX == 1 && rotZ == 0)
				rotAmounts[i] = 270.f;
			if (rotX == 1 && rotZ == 1)
				rotAmounts[i] = 180.f;

			// OUTER DEGENERATE

			// bottom (0) 
			position = glm::vec2((patchOffset - patchTranslation) * 2 - fake + posX, (patchOffset - patchTranslation) * 2 - fake + posZ);
			outerDegeneratePositions[i * 4 + 0] = position;

			// top (1)
			position = glm::vec2((patchOffset - patchTranslation) * 2 - fake + posX, (patchTranslation) * 2 - fake + posZ);
			outerDegeneratePositions[i * 4 + 1] = position;

			// right (2)
			position = glm::vec2((patchOffset - patchTranslation) * 2 - fake + posX, (patchOffset - patchTranslation) * 2 - fake + posZ);
			outerDegeneratePositions[i * 4 + 2] = position;

			// left (3)
			position = glm::vec2((patchTranslation) * 2 - fake + posX, (patchOffset - patchTranslation) * 2 - fake + posZ);
			outerDegeneratePositions[i * 4 + 3] = position;
		}

		float posX = (int)(camPos.x / 2) * 2;
		float posZ = (int)(camPos.z / 2) * 2;
		glm::vec2 position(2 - fakeDisplacement.x + posX, 2 - fakeDisplacement.z + posZ);

		// 0
		blockPositions[level * 12 + 0] = position;

		// 1
		position.y -= clipmapResolution + 1;
		blockPositions[level * 12 + 1] = position;

		// 2
		position.x -= clipmapResolution + 1;
		blockPositions[level * 12 + 2] = position;

		// 3
		position.y += clipmapResolution + 1;
		blockPositions[level * 12 + 3] = position;

		//
		//0
		position = glm::vec2(0 - fakeDisplacement.x + posX, 2 - fakeDisplacement.z + posZ);
		ringFixUpPositions[level * 4 + 0] = position;

		// 2
		position = glm::vec2(0 - fakeDisplacement.x + posX, 1 - clipmapResolution - fakeDisplacement.z + posZ);
		ringFixUpPositions[level * 4 + 2] = position;

		// 1
		position = glm::vec2(2 - fakeDisplacement.x + posX, 2 - fakeDisplacement.z + posZ);
		ringFixUpPositions[level * 4 + 1] = position;

		// 3
		position = glm::vec2(1 - clipmapResolution - fakeDisplacement.x + posX, 2 - fakeDisplacement.z + posZ);
		ringFixUpPositions[level * 4 + 3] = position;

		//
		smallSquarePosition = glm::vec2(0 - fakeDisplacement.x + posX, 0 - fakeDisplacement.z + posZ);
	}

	AABB_Box Terrain::getBoundingBoxOfClipmap(int clipmapIndex, int level) {

		glm::ivec2 blockPositionInWorldSpace = blockPositions[level * 12 + clipmapIndex];
		glm::ivec2 blockPositionInMipStack = (blockPositionInWorldSpace / MIP_STACK_DIVISOR_RATIO) % MIP_STACK_SIZE;
		int blockSizeInMipStack = clipmapResolution / MIP_STACK_DIVISOR_RATIO;
		int blockSizeInWorldSpace = clipmapResolution * (1 << level);

		unsigned char min = 255;
		unsigned char max = 0;

		int z = blockPositionInMipStack.y;
		int x = blockPositionInMipStack.x;

		for (int i = 0; i < blockSizeInMipStack; i++) {
			for (int j = 0; j < blockSizeInMipStack; j++) {

				int am = z * MIP_STACK_SIZE + x;

				if (mipStack[level][z * MIP_STACK_SIZE + x] < min)
					min = mipStack[level][z * MIP_STACK_SIZE + x];

				if (mipStack[level][z * MIP_STACK_SIZE + x] > max)
					max = mipStack[level][z * MIP_STACK_SIZE + x];

				z++;
				z %= MIP_STACK_SIZE;

				x++;
				x %= MIP_STACK_SIZE;
			}
		}

		glm::vec4 corner0(blockPositionInWorldSpace.x, min, blockPositionInWorldSpace.y, 1);
		glm::vec4 corner1(blockPositionInWorldSpace.x + blockSizeInWorldSpace, max, blockPositionInWorldSpace.y + blockSizeInWorldSpace, 1);
		AABB_Box boundingBox;
		boundingBox.start = corner0;
		boundingBox.end = corner1;
		return boundingBox;
	}

	void Terrain::streamTerrain(glm::vec3 newCamPos, int clipmapLevel) {

		for (int level = 0; level < clipmapLevel; level++) {

			glm::ivec2 old_tileIndex = Terrain::getTileIndex(level, cameraPosition);
			glm::ivec2 old_tileStart = old_tileIndex - MEM_TILE_ONE_SIDE / 2;
			glm::ivec2 old_border = old_tileStart % MEM_TILE_ONE_SIDE;
			glm::ivec2 old_clipmapPos = Terrain::getClipmapPosition(clipmapLevel, cameraPosition);

			glm::ivec2 new_tileIndex = Terrain::getTileIndex(level, newCamPos);
			glm::ivec2 new_tileStart = new_tileIndex - MEM_TILE_ONE_SIDE / 2;
			glm::ivec2 new_border = new_tileStart % MEM_TILE_ONE_SIDE;
			glm::ivec2 new_clipmapPos = Terrain::getClipmapPosition(clipmapLevel, newCamPos);

			if (old_clipmapPos != new_clipmapPos)
				for (int i = 0; i < 12; i++)
					blockAABBs[level * 12 + i] = Terrain::getBoundingBoxOfClipmap(i, level);

			glm::ivec2 tileDelta = new_tileIndex - old_tileIndex;

			if (tileDelta.x == 0 && tileDelta.y == 0)
				continue;

			if (tileDelta.x >= MEM_TILE_ONE_SIDE || tileDelta.y >= MEM_TILE_ONE_SIDE || tileDelta.x <= -MEM_TILE_ONE_SIDE || tileDelta.y <= -MEM_TILE_ONE_SIDE) {

				unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * 2];
				Terrain::loadHeightmapAtLevel(level, newCamPos, heightData);
				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(0, 0), heightData);
			//	Terrain::updateHeightsWithTerrainChunk(heights[level], heightData, glm::ivec2(0, 0), glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE * MEM_TILE_ONE_SIDE));
				delete[] heightData;
				continue;
			}

			Terrain::streamTerrainHorizontal(old_tileIndex, old_tileStart, old_border, new_tileIndex, new_tileStart, new_border, tileDelta, level);
			old_tileIndex.x = new_tileIndex.x;
			old_tileStart.x = new_tileStart.x;
			old_border.x = new_border.x;
			tileDelta.x = 0;
			Terrain::streamTerrainVertical(old_tileIndex, old_tileStart, old_border, new_tileIndex, new_tileStart, new_border, tileDelta, level);
		}

		cameraPosition = newCamPos;
	}

	void Terrain::streamTerrainHorizontal(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level) {

		if (tileDelta.x > 0) {

			old_tileStart.x += MEM_TILE_ONE_SIDE;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * 2];

			for (int x = 0; x < tileDelta.x; x++) {

				int startY = old_tileStart.y;

				std::vector<std::thread> pool;
				for (int z = 0; z < MEM_TILE_ONE_SIDE; z++) {

					std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, TILE_SIZE, glm::ivec2(old_tileStart.x, startY), glm::ivec2(0, old_border.y), heightData, old_border);
					pool.push_back(std::move(th));

					old_border.y++;
					old_border.y %= MEM_TILE_ONE_SIDE;
					startY++;
				}
				for (auto& it : pool)
					if (it.joinable())
						it.join();

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(old_border.x * TILE_SIZE, 0), heightData);
			//	Terrain::updateHeightsWithTerrainChunk(heights[level], heightData, glm::ivec2(old_border.x * TILE_SIZE, 0), glm::ivec2(TILE_SIZE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE * MEM_TILE_ONE_SIDE));

				old_border.x++;
				old_border.x %= MEM_TILE_ONE_SIDE;
				old_tileStart.x++;
			}

			delete[] heightData;
		}
		else if (tileDelta.x < 0) {

			old_tileStart.x -= 1;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * 2];

			for (int x = tileDelta.x; x < 0; x++) {

				old_border.x--;
				old_border.x += 4;
				old_border.x %= MEM_TILE_ONE_SIDE;

				int startY = old_tileStart.y;

				std::vector<std::thread> pool;
				for (int z = 0; z < MEM_TILE_ONE_SIDE; z++) {

					std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, TILE_SIZE, glm::ivec2(old_tileStart.x, startY), glm::ivec2(0, old_border.y), heightData, old_border);
					pool.push_back(std::move(th));

					old_border.y++;
					old_border.y %= MEM_TILE_ONE_SIDE;
					startY++;
				}
				for (auto& it : pool)
					if (it.joinable())
						it.join();

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(old_border.x * TILE_SIZE, 0), heightData);
				//Terrain::updateHeightsWithTerrainChunk(heights[level], heightData, glm::ivec2(old_border.x * TILE_SIZE, 0), glm::ivec2(TILE_SIZE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE * MEM_TILE_ONE_SIDE));

				old_tileStart.x--;
			}

			delete[] heightData;
		}
	}

	void Terrain::streamTerrainVertical(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level) {

		if (tileDelta.y > 0) {

			old_tileStart.y += MEM_TILE_ONE_SIDE;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * 2];

			for (int z = 0; z < tileDelta.y; z++) {

				int startX = old_tileStart.x;

				std::vector<std::thread> pool;
				for (int x = 0; x < MEM_TILE_ONE_SIDE; x++) {

					std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, MEM_TILE_ONE_SIDE * TILE_SIZE, glm::ivec2(startX, old_tileStart.y), glm::ivec2(old_border.x, 0), heightData, old_border);
					pool.push_back(std::move(th));

					old_border.x++;
					old_border.x %= MEM_TILE_ONE_SIDE;
					startX++;
				}
				for (auto& it : pool)
					if (it.joinable())
						it.join();

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE), glm::ivec2(0, old_border.y * TILE_SIZE), heightData);
			//	Terrain::updateHeightsWithTerrainChunk(heights[level], heightData, glm::ivec2(0, old_border.y * TILE_SIZE), glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE), glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE * MEM_TILE_ONE_SIDE));

				old_border.y++;
				old_border.y %= MEM_TILE_ONE_SIDE;
				old_tileStart.y++;
			}

			delete[] heightData;
		}
		else if (tileDelta.y < 0) {

			old_tileStart.y -= 1;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * 2];

			for (int z = tileDelta.y; z < 0; z++) {

				old_border.y--;
				old_border.y += 4;
				old_border.y %= MEM_TILE_ONE_SIDE;

				int startX = old_tileStart.x;

				std::vector<std::thread> pool;
				for (int x = 0; x < MEM_TILE_ONE_SIDE; x++) {

					std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, MEM_TILE_ONE_SIDE * TILE_SIZE, glm::ivec2(startX, old_tileStart.y), glm::ivec2(old_border.x, 0), heightData, old_border);
					pool.push_back(std::move(th));

					old_border.x++;
					old_border.x %= MEM_TILE_ONE_SIDE;
					startX++;
				}
				for (auto& it : pool)
					if (it.joinable())
						it.join();

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE), glm::ivec2(0, old_border.y * TILE_SIZE), heightData);
			//	Terrain::updateHeightsWithTerrainChunk(heights[level], heightData, glm::ivec2(0, old_border.y * TILE_SIZE), glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE), glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE * MEM_TILE_ONE_SIDE));

				old_tileStart.y--;
			}

			delete[] heightData;
		}
	}

	// mip stack ve height degerlerine de burda yukle
	void Terrain::loadFromDiscAndWriteGPUBufferAsync(int level, int texWidth, glm::ivec2 tileStart, glm::ivec2 border, unsigned char* heightData, glm::ivec2 toroidalUpdateBorder) {

		unsigned char* chunkHeightData = loadTerrainChunkFromDisc(level, tileStart);
		Terrain::writeHeightDataToGPUBuffer(border, texWidth, heightData, chunkHeightData, level, toroidalUpdateBorder);
		delete[] chunkHeightData;
	}

	void Terrain::updateHeightMapTextureArrayPartial(int level, glm::ivec2 size, glm::ivec2 position, unsigned char* heights) {

		GlewContext* glew = Core::instance->glewContext;
		glew->bindTexture(0x8C1A, elevationMapTexture);
		glew->texSubImage3D(0x8C1A, 0, position.x, position.y, level, size.x, size.y, 1, 0x8227, GL_UNSIGNED_BYTE, &heights[0]);
		glew->bindTexture(0x8C1A, 0);
	}

	void Terrain::deleteHeightmapArray(unsigned char** heightmapArray) {

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		for (int i = 0; i < lodLevel; i++)
			delete[] heightmapArray[i];

		delete[] heightmapArray;
	}

	void Terrain::createHeightMapTextureArray(unsigned char** heightmapArray) {

		GlewContext* glew = Core::instance->glewContext;
		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		int size = TILE_SIZE * MEM_TILE_ONE_SIDE;

		if(elevationMapTexture)
			glew->deleteTextures(1, &elevationMapTexture);

		glew->genTextures(1, &elevationMapTexture);
		glew->bindTexture(0x8C1A, elevationMapTexture);

		glew->texStorage3D(0x8C1A, 1, 0x822B, size, size, lodLevel);

		for (int i = 0; i < lodLevel; i++)
			glew->texSubImage3D(0x8C1A, 0, 0, 0, i, size, size, 1, 0x8227, GL_UNSIGNED_BYTE, &heightmapArray[i][0]);

		glew->texParameteri(0x8C1A, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glew->texParameteri(0x8C1A, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glew->texParameteri(0x8C1A, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glew->texParameteri(0x8C1A, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	void Terrain::writeHeightDataToGPUBuffer(glm::ivec2 index, int texWidth, unsigned char* heightMap, unsigned char* chunk, int level, glm::ivec2 toroidalUpdateBorder) {

		int startX = index.x * TILE_SIZE;
		int startZ = index.y * TILE_SIZE;

		int startXinHeights = toroidalUpdateBorder.x * TILE_SIZE;
		int startZinHeights = toroidalUpdateBorder.y * TILE_SIZE;

		for (int i = 0; i < TILE_SIZE; i++) {

			for (int j = 0; j < TILE_SIZE; j++) {

				int indexInChunk = (i * TILE_SIZE + j) * 2;
				int indexInHeightmap = ((i + startZ) * texWidth + j + startX) * 2;
				int indexInHeights = ((i + startZinHeights) * texWidth + j + startXinHeights) * 2;

				// Data that will be sent to GPU
				heightMap[indexInHeightmap] = chunk[indexInChunk];
				heightMap[indexInHeightmap + 1] = chunk[indexInChunk + 1];

				// Data that will be used over the time, for example terrain collision detection (not tested but probably works)
				heights[level][indexInHeights] = chunk[indexInChunk];
				heights[level][indexInHeights + 1] = chunk[indexInChunk + 1];

				// For height mip stack that will be used for frustum culling
				if (i % MIP_STACK_DIVISOR_RATIO == 0 && j % MIP_STACK_DIVISOR_RATIO == 0) {

					int indexInMipStack = ((startZinHeights / MIP_STACK_DIVISOR_RATIO) + (i / MIP_STACK_DIVISOR_RATIO)) * MIP_STACK_SIZE + (startXinHeights / MIP_STACK_DIVISOR_RATIO) + (j / MIP_STACK_DIVISOR_RATIO);
					mipStack[level][indexInMipStack] = chunk[indexInChunk];
				}
			}
		}


	}

	// okurken yaz ;)
	unsigned char* Terrain::loadTerrainChunkFromDisc(int level, glm::ivec2 index) {

		std::string path = "textures/map_" + std::to_string(level) + '_' + std::to_string(index.y) + '_' + std::to_string(index.x) + "_.png";
		std::vector<unsigned char> out;
		unsigned int w, h;
		lodepng::decode(out, w, h, path, LodePNGColorType::LCT_GREY, 16);

		unsigned char* data = new unsigned char[TILE_SIZE * TILE_SIZE * 2];

		for (int i = 0; i < out.size(); i++)
			data[i] = out[i];

		return data;
	}

	void Terrain::updateHeightsWithTerrainChunk(unsigned char* heights, unsigned char* chunk, glm::ivec2 pos, glm::ivec2 chunkSize, glm::ivec2 heightsSize) {

		for (int i = 0; i < chunkSize.y; i++) {

			for (int j = 0; j < chunkSize.x; j++) {

				int indexInChunk = (i * chunkSize.x + j) * 2;
				int indexInHeights = ((pos.y + i) * heightsSize.x + pos.x + j) * 2;
				heights[indexInHeights] = chunk[indexInChunk];
				heights[indexInHeights + 1] = chunk[indexInChunk + 1];
			}
		}
	}

	void Terrain::generateTerrainClipmapsVertexArrays() {

		GlewContext* glew = Core::instance->glewContext;
		std::vector<glm::vec3> verts;
		std::vector<glm::vec3> ringFixUpVerts;
		std::vector<glm::vec3> smallSquareVerts;
		std::vector<glm::vec3> outerDegenerateVerts;
		std::vector<glm::vec3> interiorTrimVerts;

		for (int i = 0; i < clipmapResolution; i++)
			for (int j = 0; j < clipmapResolution; j++)
				verts.push_back(glm::vec3(j, 0, i));

		for (int i = 0; i < clipmapResolution - 1; i++) {

			for (int j = 0; j < clipmapResolution - 1; j++) {

				blockIndices.push_back(j + i * (clipmapResolution));
				blockIndices.push_back(j + (i + 1) * (clipmapResolution));
				blockIndices.push_back(j + i * (clipmapResolution)+1);

				blockIndices.push_back(j + i * (clipmapResolution)+1);
				blockIndices.push_back(j + (i + 1) * (clipmapResolution));
				blockIndices.push_back(j + (i + 1) * (clipmapResolution)+1);
			}
		}

		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				smallSquareVerts.push_back(glm::vec3(j, 0, i));

		for (int i = 0; i < 3 - 1; i++) {

			for (int j = 0; j < 3 - 1; j++) {

				smallSquareIndices.push_back(j + i * (3));
				smallSquareIndices.push_back(j + (i + 1) * (3));
				smallSquareIndices.push_back(j + i * (3) + 1);

				smallSquareIndices.push_back(j + i * (3) + 1);
				smallSquareIndices.push_back(j + (i + 1) * (3));
				smallSquareIndices.push_back(j + (i + 1) * (3) + 1);
			}
		}

		for (int i = 0; i < clipmapResolution; i++)
			for (int j = 0; j < 3; j++)
				ringFixUpVerts.push_back(glm::vec3(j, 0, i));

		for (int i = 0; i < clipmapResolution - 1; i++) {

			for (int j = 0; j < 3 - 1; j++) {

				ringFixUpIndices.push_back(j + i * (3));
				ringFixUpIndices.push_back(j + (i + 1) * (3));
				ringFixUpIndices.push_back(j + i * (3) + 1);

				ringFixUpIndices.push_back(j + i * (3) + 1);
				ringFixUpIndices.push_back(j + (i + 1) * (3));
				ringFixUpIndices.push_back(j + (i + 1) * (3) + 1);
			}
		}

		for(int i = 0; i <= clipmapResolution * 4 - 2; i++)
			outerDegenerateVerts.push_back(glm::vec3(i, 0, 0));

		for (int i = 0; i < clipmapResolution * 2 - 1; i++) {
			outerDegenerateIndices.push_back(i * 2);
			outerDegenerateIndices.push_back(i * 2 + 2);
			outerDegenerateIndices.push_back(i * 2 + 1);
		}

		for (int i = 0; i < clipmapResolution * 2 - 1; i++) {
			outerDegenerateIndices.push_back(i * 2);
			outerDegenerateIndices.push_back(i * 2 + 1);
			outerDegenerateIndices.push_back(i * 2 + 2);
		}

		int size = clipmapResolution * 2;

		for (int i = -size / 2; i <= size / 2; i++)
			interiorTrimVerts.push_back(glm::vec3(i, 0, size / 2 - 1));

		for (int i = -size / 2; i <= size / 2; i++)
			interiorTrimVerts.push_back(glm::vec3(i, 0, size / 2));

		for (int i = size / 2 - 2; i >= -size / 2; i--)
			interiorTrimVerts.push_back(glm::vec3(size / 2 - 1, 0, i));

		for (int i = size / 2 - 2; i >= -size / 2; i--)
			interiorTrimVerts.push_back(glm::vec3(size / 2, 0, i));

		/*
		* INTERIOR TRIM INDICES
		* 
		* 9   8   7   6   5  
		* 4   3   2   1   0
		* 13 10
		* 14 11
		* 15 12
		*/

		for (int i = 0; i < size; i++) {

			interiorTrimIndices.push_back(i);
			interiorTrimIndices.push_back(i + size + 1);
			interiorTrimIndices.push_back(i + 1);

			interiorTrimIndices.push_back(i + 1);
			interiorTrimIndices.push_back(i + size + 1);
			interiorTrimIndices.push_back(i + size + 2);
		}

		interiorTrimIndices.push_back(size - 1);
		interiorTrimIndices.push_back(size);
		interiorTrimIndices.push_back(size * 3 + 1);

		interiorTrimIndices.push_back(size - 1);
		interiorTrimIndices.push_back(size * 3 + 1);
		interiorTrimIndices.push_back(size * 2 + 2);

		for (int i = 0; i < size - 2; i++) {

			interiorTrimIndices.push_back(i + size * 2 + 2);
			interiorTrimIndices.push_back(i + size * 3 + 1);
			interiorTrimIndices.push_back(i + size * 3 + 2);

			interiorTrimIndices.push_back(i + size * 2 + 2);
			interiorTrimIndices.push_back(i + size * 3 + 2);
			interiorTrimIndices.push_back(i + size * 2 + 3);
		}

		// Block
		glew->genVertexArrays(1, &blockVAO);
		glew->bindVertexArray(blockVAO);

		unsigned int VBO;
		glew->genBuffers(1, &VBO);
		glew->bindBuffer(0x8892, VBO);
		glew->bufferData(0x8892, verts.size() * sizeof(glm::vec3), &verts[0], 0x88E4);
		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int EBO;
		glew->genBuffers(1, &EBO);
		glew->bindBuffer(0x8893, EBO);
		glew->bufferData(0x8893, blockIndices.size() * sizeof(unsigned int), &blockIndices[0], 0x88E4);

		// Ring Fix-up
		glew->genVertexArrays(1, &ringFixUpVAO);
		glew->bindVertexArray(ringFixUpVAO);

		unsigned int ringFixUpVBO;
		glew->genBuffers(1, &ringFixUpVBO);
		glew->bindBuffer(0x8892, ringFixUpVBO);
		glew->bufferData(0x8892, ringFixUpVerts.size() * sizeof(glm::vec3), &ringFixUpVerts[0], 0x88E4);
		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int ringFixUpEBO;
		glew->genBuffers(1, &ringFixUpEBO);
		glew->bindBuffer(0x8893, ringFixUpEBO);
		glew->bufferData(0x8893, ringFixUpIndices.size() * sizeof(unsigned int), &ringFixUpIndices[0], 0x88E4);

		// Small Square
		glew->genVertexArrays(1, &smallSquareVAO);
		glew->bindVertexArray(smallSquareVAO);

		unsigned int smallSquareVBO;
		glew->genBuffers(1, &smallSquareVBO);
		glew->bindBuffer(0x8892, smallSquareVBO);
		glew->bufferData(0x8892, smallSquareVerts.size() * sizeof(glm::vec3), &smallSquareVerts[0], 0x88E4);
		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int smallSquareEBO;
		glew->genBuffers(1, &smallSquareEBO);
		glew->bindBuffer(0x8893, smallSquareEBO);
		glew->bufferData(0x8893, smallSquareIndices.size() * sizeof(unsigned int), &smallSquareIndices[0], 0x88E4);

		// Outer Degenerate
		glew->genVertexArrays(1, &outerDegenerateVAO);
		glew->bindVertexArray(outerDegenerateVAO);

		unsigned int outerDegenerateVBO;
		glew->genBuffers(1, &outerDegenerateVBO);
		glew->bindBuffer(0x8892, outerDegenerateVBO);
		glew->bufferData(0x8892, outerDegenerateVerts.size() * sizeof(glm::vec3), &outerDegenerateVerts[0], 0x88E4);
		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int outerDegenerateEBO;
		glew->genBuffers(1, &outerDegenerateEBO);
		glew->bindBuffer(0x8893, outerDegenerateEBO);
		glew->bufferData(0x8893, outerDegenerateIndices.size() * sizeof(unsigned int), &outerDegenerateIndices[0], 0x88E4);

		// Interior Trim
		glew->genVertexArrays(1, &interiorTrimVAO);
		glew->bindVertexArray(interiorTrimVAO);

		unsigned int interiorTrimVBO;
		glew->genBuffers(1, &interiorTrimVBO);
		glew->bindBuffer(0x8892, interiorTrimVBO);
		glew->bufferData(0x8892, interiorTrimVerts.size() * sizeof(glm::vec3), &interiorTrimVerts[0], 0x88E4);
		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int interiorTrimEBO;
		glew->genBuffers(1, &interiorTrimEBO);
		glew->bindBuffer(0x8893, interiorTrimEBO);
		glew->bufferData(0x8893, interiorTrimIndices.size() * sizeof(unsigned int), &interiorTrimIndices[0], 0x88E4);

		glew->bindVertexArray(0);
	}

	glm::ivec2 Terrain::getClipmapPosition(int level, glm::vec3& camPos) {

		int patchWidth = 2;
		float patchOffset = 1 << level;
		float requiredCameraDisplacement = patchWidth * patchOffset;
		float posX = (int)(camPos.x / requiredCameraDisplacement) * requiredCameraDisplacement;
		float posZ = (int)(camPos.z / requiredCameraDisplacement) * requiredCameraDisplacement;
		return glm::ivec2(posX + patchOffset, posZ + patchOffset);
	}

	glm::ivec2 Terrain::getTileIndex(int level, glm::vec3& camPos) {

		int parcelSizeInReal = TILE_SIZE * (1 << level);
		glm::ivec2 clipmapPos = Terrain::getClipmapPosition(level, camPos);
		return glm::ivec2(clipmapPos.x / parcelSizeInReal, clipmapPos.y / parcelSizeInReal);
	}

	int Terrain::getMaxMipLevel(int textureSize, int tileSize) {

		int lodLevel = 0;
		int num = tileSize;
		while (num <= textureSize) {
			num *= 2;
			lodLevel++;
		}
		//return lodLevel;
		return 3;
	}

}