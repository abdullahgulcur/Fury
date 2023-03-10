#include "pch.h"
#include "terrain.h"
#include "terrain.h"
#include "core.h"
#include "scenemanager.h"
#include "scene.h"
#include "glewcontext.h"
#include "lodepng/lodepng.h"

using namespace std::chrono;

namespace Fury {

	Terrain::Terrain() {

		Terrain::initTilesPtr();
	}

	Terrain::~Terrain() {

		GlewContext* glew = Core::instance->glewContext;
		glew->deleteTextures(1, &elevationMapTexture);
		glew->deleteVertexArrays(1, &blockVAO);
		glew->deleteVertexArrays(1, &ringFixUpVAO);
		glew->deleteVertexArrays(1, &smallSquareVAO);
		glew->deleteVertexArrays(1, &outerDegenerateVAO);
		glew->deleteVertexArrays(1, &interiorTrimVAO);

		Terrain::deleteAllTiles();

		delete[] blockPositions;
		delete[] ringFixUpPositions;
		delete[] interiorTrimPositions;
		delete[] outerDegeneratePositions;
		delete[] rotAmounts;
		delete[] blockAABBs;

		Terrain::deleteAllLowDetailMipStack();
	}

	void Terrain::initTilesPtr() {

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		tiles = new unsigned char** [lodLevel];

		for (int i = 0; i < lodLevel; i++)
			tiles[i] = new unsigned char* [MEM_TILE_ONE_SIDE * MEM_TILE_ONE_SIDE];
	}

	void Terrain::update() {

		int level = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		int clipmapResolution = this->clipmapResolution;

		// Change this for debugging purposes--------
		//glm::vec3 camPos = camera->position;
		float fake = 1000000;
		glm::vec3 fakeDisplacement = glm::vec3(fake, 0, fake);
		glm::vec3 camPosition = Core::instance->sceneManager->currentScene->primaryCamera->position;
		glm::vec3 camPos = camPosition + fakeDisplacement;
		//-------------------------------------------

		// It has to be two. 
		int patchWidth = 2;

		// '4' has to be constant, because every level has 4 block at each side.
		int wholeClipmapRegionSize = clipmapResolution * 4 * (1 << level);



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
			//clipmapPositions[i].z = position.y - (1 << i); // bunlar disarda tutulcak cunku moduler olmali
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

			Terrain::setClipmapPositionVertical(i, position.y - (1 << i));

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

			Terrain::setClipmapPositionHorizontal(i, position.x - (1 << i));



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

		//
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

	void Terrain::calculateTerrainChunkPositions() {

		int level = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		int clipmapResolution = this->clipmapResolution;

		// Change this for debugging purposes--------
		//glm::vec3 camPos = camera->position;
		float fake = 1000000;
		glm::vec3 fakeDisplacement = glm::vec3(fake, 0, fake);
		glm::vec3 camPosition = Core::instance->sceneManager->currentScene->primaryCamera->position;
		glm::vec3 camPos = camPosition + fakeDisplacement;
		//-------------------------------------------

		// It has to be two. 
		int patchWidth = 2;

		// '4' has to be constant, because every level has 4 block at each side.
		int wholeClipmapRegionSize = clipmapResolution * 4 * (1 << level);

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

		//
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

	void Terrain::init() {

		GameCamera* camera = Core::instance->sceneManager->currentScene->primaryCamera;
		GlewContext* glew = Core::instance->glewContext;
		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);

		blockPositions = new glm::vec2[12 * lodLevel + 4];
		ringFixUpPositions = new glm::vec2[4 * lodLevel + 4];
		interiorTrimPositions = new glm::vec2[lodLevel];
		outerDegeneratePositions = new glm::vec2[4 * lodLevel];
		rotAmounts = new float[lodLevel];
		blockAABBs = new AABB_Box[12 * lodLevel + 4];
		//ringfixupAABBs = new AABB_Box[4 * lodLevel + 4];

		Terrain::calculateTerrainChunkPositions();

		for (int i = 0; i < lodLevel; i++) {

			Vec2Int clipmapPos = Terrain::getClipmapPosition(i, camera->position);
			clipmapPositions.push_back(clipmapPos);

			int parcelSizeInReal = TILE_SIZE * (1 << i);
			currentTileIndices.push_back(Vec2Int(clipmapPos.x / parcelSizeInReal, clipmapPos.z / parcelSizeInReal));

			//(float)terrain->currentTileIndices[i].x * TILE_SIZE * (1 << i), (float)terrain->currentTileIndices[i].z * TILE_SIZE * (1 << i)
			initialTexturePositions.push_back(Vec2Int((float)currentTileIndices[i].x * TILE_SIZE * (1 << i),
				(float)currentTileIndices[i].z * TILE_SIZE * (1 << i)));

			toroidalUpdateIndices.push_back(Vec2Int(0, 0));

		}

		//Terrain::createDummyHeightmapTexture(TILE_SIZE, RESOLUTION);
		Terrain::loadAllHeightmaps();
		unsigned char** heightData = NULL; // 0: level, 1: byte
		Terrain::sendHeightDataToGraphicsMemory(lodLevel, heightData);
		Terrain::loadAllLowDetailMipStack(lodLevel);
		Terrain::createHeightMapTextureArray(heightData);
		Terrain::deleteAllHeightData(heightData);

		for (int i = 0; i < lodLevel; i++) {

			/// <summary>
			/// fonksiyona al
			/// </summary>
			for (int j = 0; j < 12; j++) {

				glm::vec4 startInWorldSpace;
				glm::vec4 endInWorldSpace;
				Terrain::getTerrainChunkBoundaries(blockPositions[i * 12 + j].x, blockPositions[i * 12 + j].y, i, startInWorldSpace, endInWorldSpace);
				AABB_Box aabb;
				aabb.start = startInWorldSpace;
				aabb.end = endInWorldSpace;
				blockAABBs[i * 12 + j] = aabb;
			}

			//for (int j = 0; j < 4; j++) {

			//	glm::vec4 startInWorldSpace;
			//	glm::vec4 endInWorldSpace;
			//	Terrain::getTerrainChunkBoundaries(ringFixUpPositions[i * 4 + j].x, ringFixUpPositions[i * 4 + j].y, i, startInWorldSpace, endInWorldSpace);
			//	AABB_Box aabb;
			//	aabb.start = startInWorldSpace;
			//	aabb.end = endInWorldSpace;
			//	ringfixupAABBs[i * 12 + j] = aabb;
			//}
		}
	    
		//std::vector<unsigned char> out(4 * TILE_SIZE * 4 * TILE_SIZE * 2);
		//for (int i = 0; i < 4 * TILE_SIZE * 4 * TILE_SIZE * 2; i++)
		//	out[i] = heightData[0][i];


		//unsigned int width = 4 * TILE_SIZE;
		//unsigned int height = 4 * TILE_SIZE;
		//std::string imagePath = "testt.png";
		//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
		//int x = 5;


		//Terrain::createHeightMapTexture(heightData[1]);

		//delete[] heightData;
		//std::vector<float> heightImage(elevationMapSize * elevationMapSize);
		//std::vector<char> normalImage(elevationMapSize * elevationMapSize * 3, 0);


		/*Terrain::createHeightMap(heightImage);
		Terrain::recalculateNormals(heightImage, normalImage);
		Terrain::createNormalMap(normalImage);*/
		Terrain::generateTerrainClipmapsVertexArrays();

		programID = glew->loadShaders("C:/Projects/Fury/Core/src/shader/clipmap.vert",
			"C:/Projects/Fury/Core/src/shader/clipmap.frag");
		glew->useProgram(programID);
		glew->uniform1i(glew->getUniformLocation(programID, "irradianceMap"), 0);
		glew->uniform1i(glew->getUniformLocation(programID, "prefilterMap"), 1);
		glew->uniform1i(glew->getUniformLocation(programID, "brdfLUT"), 2);
		glew->uniform1i(glew->getUniformLocation(programID, "texture0"), 3);
		glew->uniform1i(glew->getUniformLocation(programID, "texture1"), 4);
		glew->uniform1i(glew->getUniformLocation(programID, "texture2"), 5);
		glew->uniform1i(glew->getUniformLocation(programID, "texture3"), 6);
		glew->uniform1i(glew->getUniformLocation(programID, "texture4"), 7);
		glew->uniform1i(glew->getUniformLocation(programID, "texture5"), 8);
		glew->uniform1i(glew->getUniformLocation(programID, "heightmapArray"), 9);

		unsigned int width;
		unsigned int height;
		std::vector<unsigned char> image;
		lodepng::decode(image, width, height, "grass_a.png");
		glew->generateTexture(diffuse, width, height, &image[0]);
		image.clear();

		lodepng::decode(image, width, height, "grass_n.png");
		glew->generateTexture(normal, width, height, &image[0]);
		image.clear();

		lodepng::decode(image, width, height, "grass_m.png");
		glew->generateTexture(metalness, width, height, &image[0]);
		image.clear();

		lodepng::decode(image, width, height, "grass_r.png");
		glew->generateTexture(roughness, width, height, &image[0]);
		image.clear();

		lodepng::decode(image, width, height, "grass_ao.png");
		glew->generateTexture(ao, width, height, &image[0]);
		image.clear();

		lodepng::decode(image, width, height, "grass_d.png");
		glew->generateTexture(displacement, width, height, &image[0]);
		image.clear();
	}

	void Terrain::loadAllLowDetailMipStack(int lodLevel) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		int divisionMultiplier = 8;

		lowDetailMipStack = new unsigned char* [lodLevel]; 

		for (int i = 0; i < lodLevel; i++)
			lowDetailMipStack[i] = new unsigned char[(gpuParcels * gpuParcels * TILE_SIZE * TILE_SIZE * 2) / (divisionMultiplier * divisionMultiplier)];

		for (int level = 0; level < lodLevel; level++)
			for (int i = 1; i < MEM_TILE_ONE_SIDE - 1; i++)
				for (int j = 1; j < MEM_TILE_ONE_SIDE - 1; j++)
					Terrain::writeHeightDataToMipStack(level, lowDetailMipStack[level], j, i, divisionMultiplier);
	}

	void Terrain::deleteAllLowDetailMipStack() {

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);

		for (int i = 0; i < lodLevel; i++)
			delete lowDetailMipStack[i];

		delete[] lowDetailMipStack;
	}

	void Terrain::deleteLowDetailMipStack(int lodLevel) {

		delete[] lowDetailMipStack[lodLevel];
		lowDetailMipStack[lodLevel] = NULL;
	}

	void Terrain::loadLowDetailMipStackAtLevel(int lodLevel) {

		int divisionMultiplier = 8;

		for (int i = 1; i < MEM_TILE_ONE_SIDE - 1; i++)
			for (int j = 1; j < MEM_TILE_ONE_SIDE - 1; j++)
				Terrain::writeHeightDataToMipStack(lodLevel, lowDetailMipStack[lodLevel], j, i, divisionMultiplier);
	}

	void Terrain::loadAllHeightmaps() {

		int offset = MEM_TILE_ONE_SIDE / 2;
		int resolution = RESOLUTION;
		int tileSize = TILE_SIZE;
		int lodLevel = Terrain::getMaxMipLevel(resolution, tileSize);

		for (int level = 0; level < lodLevel; level++) {
			for (int i = 0; i < MEM_TILE_ONE_SIDE; i++) {
				for (int j = 0; j < MEM_TILE_ONE_SIDE; j++) {

					int x = currentTileIndices[level].x;
					int z = currentTileIndices[level].z;
					tiles[level][i * MEM_TILE_ONE_SIDE + j] = Terrain::loadHeightmapFromDisk(level, x + j - offset, z + i - offset);
				}
			}
		}
	}

	void Terrain::sendHeightDataToGraphicsMemory(int lodLevel, unsigned char**& heightData) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		heightData = new unsigned char*[lodLevel];

		for (int i = 0; i < lodLevel; i++)
			heightData[i] = new unsigned char[gpuParcels * gpuParcels * TILE_SIZE * TILE_SIZE * 2];

		// make it multi-threaded
		for (int level = 0; level < lodLevel; level++)
			for (int i = 1; i < MEM_TILE_ONE_SIDE - 1; i++)
				for (int j = 1; j < MEM_TILE_ONE_SIDE - 1; j++)
					Terrain::writeHeightDataToGPUBuffer(level, heightData[level], j, i);

		//std::vector<unsigned char> out(gpuParcels * TILE_SIZE * gpuParcels * TILE_SIZE * 2);
		//for (int i = 0; i < gpuParcels * TILE_SIZE * gpuParcels * TILE_SIZE * 2; i++)
		//	out[i] = heightData[1][i];


		//unsigned int width = gpuParcels * TILE_SIZE;
		//unsigned int height = gpuParcels * TILE_SIZE;
		//std::string imagePath = "test1.png";
		//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
		//int x = 5;
	}

	void Terrain::loadTerrainChunkOnGraphicsMemoryVertical(int lodLevel, unsigned char*& heightData, int dir) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		heightData = new unsigned char[gpuParcels * TILE_SIZE * TILE_SIZE * 2];

		//std::vector<std::thread> pool;
		// make it multi-threaded
		for (int i = 1; i < MEM_TILE_ONE_SIDE - 1; i++) {

			// update this with mathematical func
			if (dir == -1) {

				//std::thread th(&Terrain::writeHeightDataToGPUBufferVertical, this, lodLevel, heightData, i, 1);
				//pool.push_back(std::move(th));
				Terrain::writeHeightDataToGPUBufferVertical(lodLevel, heightData, i, 1);
			}

			if (dir == 1) {
				//std::thread th(&Terrain::writeHeightDataToGPUBufferVertical, this, lodLevel, heightData, i, gpuParcels);
				//pool.push_back(std::move(th));
				Terrain::writeHeightDataToGPUBufferVertical(lodLevel, heightData, i, gpuParcels);
			}
		}

		//for (auto& it : pool)
		//	if (it.joinable())
		//		it.join();
	}

	void Terrain::loadTerrainChunkOnGraphicsMemoryHorizontal(int lodLevel, unsigned char*& heightData, int dir) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		heightData = new unsigned char[gpuParcels * TILE_SIZE * TILE_SIZE * 2];

		for (int i = 1; i < MEM_TILE_ONE_SIDE - 1; i++) {

			if (dir == -1) {
				Terrain::writeHeightDataToGPUBufferHorizontal(lodLevel, heightData, 1, i);
			}

			if (dir == 1) {
				Terrain::writeHeightDataToGPUBufferHorizontal(lodLevel, heightData, gpuParcels, i);
			}
		}
	}

	void Terrain::updateHeightDataOnGraphicsMemory(int lodLevel, unsigned char*& heightData) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		heightData = new unsigned char[gpuParcels * gpuParcels * TILE_SIZE * TILE_SIZE * 2];

		// make it multi-threaded
		for (int i = 1; i < MEM_TILE_ONE_SIDE - 1; i++)
			for (int j = 1; j < MEM_TILE_ONE_SIDE - 1; j++)
				Terrain::writeHeightDataToGPUBuffer(lodLevel, heightData, j, i);

		//std::vector<unsigned char> out(gpuParcels * TILE_SIZE * gpuParcels * TILE_SIZE * 2);
		//for (int i = 0; i < gpuParcels * TILE_SIZE * gpuParcels * TILE_SIZE * 2; i++)
		//	out[i] = heightData[1][i];


		//unsigned int width = gpuParcels * TILE_SIZE;
		//unsigned int height = gpuParcels * TILE_SIZE;
		//std::string imagePath = "test1.png";
		//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
		//int x = 5;
	}

	void Terrain::writeHeightDataToGPUBufferVertical(int level, unsigned char* buffer, int z, int x) {

		int texWidth = TILE_SIZE;
		//int startZ = (z - 1) * TILE_SIZE;
		int startZ = ((toroidalUpdateIndices[level].z + z - 1) % 4) * TILE_SIZE;

		for (int i = 0; i < TILE_SIZE; i++) {
			for (int j = 0; j < TILE_SIZE; j++) {

				int indexInGPUBuffer = ((i + startZ) * texWidth + j) * 2;
				int indexInTile = (i * TILE_SIZE + j) * 2;
				int tileIndex = z * MEM_TILE_ONE_SIDE + x;
				buffer[indexInGPUBuffer] = tiles[level][tileIndex][indexInTile];
				buffer[indexInGPUBuffer + 1] = tiles[level][tileIndex][indexInTile + 1];
			}
		}
	}

	void Terrain::writeHeightDataToGPUBufferHorizontal(int level, unsigned char* buffer, int z, int x) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		int texWidth = TILE_SIZE * gpuParcels;
		//int startX = (x - 1) * TILE_SIZE;
		int startX = ((toroidalUpdateIndices[level].x + x - 1) % 4) * TILE_SIZE;

		for (int i = 0; i < TILE_SIZE; i++) {
			for (int j = 0; j < TILE_SIZE; j++) {

				int indexInGPUBuffer = (i * texWidth + j + startX) * 2;
				int indexInTile = (i * TILE_SIZE + j) * 2;
				int tileIndex = z * MEM_TILE_ONE_SIDE + x;
				buffer[indexInGPUBuffer] = tiles[level][tileIndex][indexInTile];
				buffer[indexInGPUBuffer + 1] = tiles[level][tileIndex][indexInTile + 1];
			}
		}
	}

	void Terrain::writeHeightDataToMipStack(int level, unsigned char*& buffer, int x, int z, int divisionMultiplier) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		int texSizeMipmap = (gpuParcels * TILE_SIZE) / divisionMultiplier;
		int texSize = TILE_SIZE / divisionMultiplier;
		int startZ = (z - 1) * (TILE_SIZE / divisionMultiplier);
		int startX = (x - 1) * (TILE_SIZE / divisionMultiplier);

		for (int i = 0; i < texSize; i++) {
			for (int j = 0; j < texSize; j++) {

				int indexInMipmap = ((i + startZ) * texSizeMipmap + (j + startX)) * 2;
				int indexInTile = (i * TILE_SIZE + j ) * divisionMultiplier * 2;
				int tileIndex = z * MEM_TILE_ONE_SIDE + x;
				buffer[indexInMipmap] = tiles[level][tileIndex][indexInTile];
				buffer[indexInMipmap + 1] = tiles[level][tileIndex][indexInTile + 1];
			}
		}
	}

	void Terrain::writeHeightDataToGPUBuffer(int level, unsigned char*& buffer, int x, int z) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		int texSize = gpuParcels * TILE_SIZE;
		int startZ = (z - 1) * TILE_SIZE;
		int startX = (x - 1) * TILE_SIZE;

		for (int i = 0; i < TILE_SIZE; i++) {
			for (int j = 0; j < TILE_SIZE; j++) {
				
				int indexInGPUBuffer = ((i + startZ) * texSize + (j + startX)) * 2;
				int indexInTile = (i * TILE_SIZE + j) * 2;
				int tileIndex = z * MEM_TILE_ONE_SIDE + x;
				buffer[indexInGPUBuffer] = tiles[level][tileIndex][indexInTile];
				buffer[indexInGPUBuffer + 1] = tiles[level][tileIndex][indexInTile + 1];
			}
		}

		//std::vector<unsigned char> out(gpuParcels * TILE_SIZE * gpuParcels * TILE_SIZE * 2);
		//for (int i = 0; i < gpuParcels * TILE_SIZE * gpuParcels * TILE_SIZE * 2; i++)
		//	out[i] = buffer[i];


		//unsigned int width = gpuParcels * TILE_SIZE;
		//unsigned int height = gpuParcels * TILE_SIZE;
		//std::string imagePath = "testt.png";
		//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
		//x = 5;

		
	}

	//lode png kullanma, freeimage daha hizli
	unsigned char* Terrain::loadHeightmapFromDisk(int level, int x, int z) {

		std::string path = "textures/map_" + std::to_string(level) + '_' + std::to_string(z) + '_' + std::to_string(x) + "_.png";
		std::vector<unsigned char> out;
		unsigned int w;
		unsigned int h;
		lodepng::decode(out, w, h, path, LodePNGColorType::LCT_GREY, 16);

		unsigned char* data = new unsigned char[TILE_SIZE * TILE_SIZE * 2];

		for (int i = 0; i < out.size(); i++)
			data[i] = out[i];

		return data;
	}

	void Terrain::loadAllHeightmapNames() {

		//for (int i = 0; i < 8; i++) {
		//	for (int j = 0; j < 8; j++) {

		//		std::string name = "map_" + std::to_string(i) + '_' + std::to_string(j) + "_.png";
		//		heightmapNameList.push_back(name);
		//	}
		//}
	}

	void Terrain::createHeightMapTexture(unsigned char* heights) {

		GlewContext* glew = Core::instance->glewContext;
		glew->genTextures(1, &elevationMapTexture);
		glew->bindTexture(0x0DE1, elevationMapTexture);
		glew->texParameteri(0x0DE1, 0x2802, 0x2901);
		glew->texParameteri(0x0DE1, 0x2803, 0x2901);
		glew->texParameteri(0x0DE1, 0x2800, 0x2601);
		glew->texParameteri(0x0DE1, 0x2801, 0x2601);

		// load image, create texture and generate mipmaps

		int size = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2);
		glew->texImage2D(GL_TEXTURE_2D, 0, 0x8227, size, size, 0, 0x8227, 0x1401, heights);
		glew->generateMipmap(0x0DE1);
	}

	void Terrain::createHeightMapTextureArray(unsigned char** heights) {

		GlewContext* glew = Core::instance->glewContext;
		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		int size = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2);

		glew->genTextures(1, &elevationMapTexture);
		glew->bindTexture(0x8C1A, elevationMapTexture);

		glew->texStorage3D(0x8C1A, 1, 0x822B, size, size, lodLevel);

		for (int i = 0; i < lodLevel; i++)
			glew->texSubImage3D(0x8C1A, 0, 0, 0, i, size, size, 1, 0x8227, GL_UNSIGNED_BYTE, &heights[i][0]);

		glew->texParameteri(0x8C1A, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glew->texParameteri(0x8C1A, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glew->texParameteri(0x8C1A, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glew->texParameteri(0x8C1A, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	void Terrain::updateHeightMapTextureArray(int level, unsigned char* heights) {

		GlewContext* glew = Core::instance->glewContext;
		int size = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2);
		glew->bindTexture(0x8C1A, elevationMapTexture);
		glew->texSubImage3D(0x8C1A, 0, 0, 0, level, size, size, 1, 0x8227, GL_UNSIGNED_BYTE, &heights[0]);
		glew->bindTexture(0x8C1A, 0);
	}

	void Terrain::updateHeightMapTextureArrayPartialVertical(int level, unsigned char* heights) {

		int pos = toroidalUpdateIndices[level].x * TILE_SIZE;

		GlewContext* glew = Core::instance->glewContext;
		int size = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2);
		glew->bindTexture(0x8C1A, elevationMapTexture);
		glew->texSubImage3D(0x8C1A, 0, pos, 0, level, TILE_SIZE, size, 1, 0x8227, GL_UNSIGNED_BYTE, &heights[0]);
		glew->bindTexture(0x8C1A, 0);
	}

	void Terrain::updateHeightMapTextureArrayPartialHorizontal(int level, unsigned char* heights) {

		int pos = toroidalUpdateIndices[level].z * TILE_SIZE;

		GlewContext* glew = Core::instance->glewContext;
		int size = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2);
		glew->bindTexture(0x8C1A, elevationMapTexture);
		glew->texSubImage3D(0x8C1A, 0, 0, pos, level, size, TILE_SIZE, 1, 0x8227, GL_UNSIGNED_BYTE, &heights[0]);
		glew->bindTexture(0x8C1A, 0);
	}

	void Terrain::recreateHeightMapTextureArray(unsigned char** heights) {

		GlewContext* glew = Core::instance->glewContext;
		glew->deleteTextures(1, &elevationMapTexture);
		Terrain::createHeightMapTextureArray(heights);
	}

	void Terrain::recreateHeightMapTexture(unsigned char* heights) {

		GlewContext* glew = Core::instance->glewContext;
		glew->deleteTextures(1, &elevationMapTexture);
		Terrain::createHeightMapTexture(heights);
	}

	void Terrain::createHeightMap(std::vector<float>& heightImage) {//

		GlewContext* glew = Core::instance->glewContext;
		glew->genTextures(1, &elevationMapTexture);
		glew->bindTexture(0x0DE1, elevationMapTexture);
		glew->texParameteri(0x0DE1, 0x2802, 0x2901);
		glew->texParameteri(0x0DE1, 0x2803, 0x2901);
		glew->texParameteri(0x0DE1, 0x2800, 0x2601);
		glew->texParameteri(0x0DE1, 0x2801, 0x2601);

		// load image, create texture and generate mipmaps
		
		Terrain::createHeightMapWithPerlinNoise(heightImage);
		//glew->texImage2D(0x0DE1, 0, 0x822E, elevationMapSize, elevationMapSize, 0, 0x1903, 0x1406, &heightImage[0]);
		glew->generateMipmap(0x0DE1);
	}

	void Terrain::createNormalMap(std::vector<char>& normalImage) {//

		GlewContext* glew = Core::instance->glewContext;
		glew->genTextures(1, &normalMapTexture);
		glew->bindTexture(0x0DE1, normalMapTexture);
		glew->texParameteri(0x0DE1, 0x2802, 0x2901);
		glew->texParameteri(0x0DE1, 0x2803, 0x2901);
		glew->texParameteri(0x0DE1, 0x2800, 0x2601);
		glew->texParameteri(0x0DE1, 0x2801, 0x2601);
		//glew->texImage2D(0x0DE1, 0, 0x1907, elevationMapSize, elevationMapSize, 0, 0x1907, 0x1400, &normalImage[0]);
		glew->generateMipmap(0x0DE1);
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

		// Ring Fix-up (Horizontal)
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

	void Terrain::setClipmapPositionHorizontal(int level, int x) {

		if (clipmapPositions[level].x != x) {

			int parcelSizeInReal = TILE_SIZE * (1 << level);
			int currentTileIndex = x / parcelSizeInReal;

			if (currentTileIndices[level].x != currentTileIndex) {

				int offset = currentTileIndex - currentTileIndices[level].x;

				if (offset == 1) {

					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++)
						delete[] tiles[level][i * MEM_TILE_ONE_SIDE];

					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++)
						for (int j = 0; j < MEM_TILE_ONE_SIDE - 1; j++)
							tiles[level][i * MEM_TILE_ONE_SIDE + j] = tiles[level][i * MEM_TILE_ONE_SIDE + j + 1];

					//auto start = high_resolution_clock::now();
					std::vector<std::thread> pool;

					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++) {
						int offset = MEM_TILE_ONE_SIDE / 2;
						std::thread th(&Terrain::loadMapFromDiscAsync, this, level, (i + 1) * MEM_TILE_ONE_SIDE - 1, currentTileIndex + offset - 1, currentTileIndices[level].z - offset + i);
						pool.push_back(std::move(th));
					}

					for (auto& it : pool)
						if (it.joinable())
							it.join();

					//auto stop = high_resolution_clock::now();
					//auto duration = duration_cast<microseconds>(stop - start);
					//std::cout << "Time taken by disc load function: " << duration.count() << " microseconds" << std::endl;

					//start = high_resolution_clock::now();

					Terrain::loadLowDetailMipStackAtLevel(level);

					//stop = high_resolution_clock::now();
					//duration = duration_cast<microseconds>(stop - start);
					//std::cout << "Time taken by mipstack load function: " << duration.count() << " microseconds" << std::endl;

					//start = high_resolution_clock::now();

					unsigned char* heightData = NULL;
					Terrain::loadTerrainChunkOnGraphicsMemoryVertical(level, heightData, 1);
					Terrain::updateHeightMapTextureArrayPartialVertical(level, heightData);
					delete[] heightData;

					//stop = high_resolution_clock::now();
					//duration = duration_cast<microseconds>(stop - start);
					//std::cout << "Time taken by gpu load function: " << duration.count() << " microseconds" << std::endl;

					toroidalUpdateIndices[level].x++;
					toroidalUpdateIndices[level].x = (toroidalUpdateIndices[level].x + (MEM_TILE_ONE_SIDE - 2)) % (MEM_TILE_ONE_SIDE - 2);
				}
				else if (offset == -1) {

					toroidalUpdateIndices[level].x--;
					toroidalUpdateIndices[level].x = (toroidalUpdateIndices[level].x + (MEM_TILE_ONE_SIDE - 2)) % (MEM_TILE_ONE_SIDE - 2);

					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++)
						delete[] tiles[level][(i + 1) * MEM_TILE_ONE_SIDE - 1];

					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++)
						for (int j = MEM_TILE_ONE_SIDE - 1; j >= 0; j--)
							tiles[level][i * MEM_TILE_ONE_SIDE + j] = tiles[level][i * MEM_TILE_ONE_SIDE + j - 1];

					
					//auto start = high_resolution_clock::now();

					std::vector<std::thread> pool;
					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++) {
						int offset = MEM_TILE_ONE_SIDE / 2;
						std::thread th(&Terrain::loadMapFromDiscAsync, this, level, i * MEM_TILE_ONE_SIDE, currentTileIndex - offset, currentTileIndices[level].z - offset + i);
						pool.push_back(std::move(th));
					}

					for (auto& it : pool)
						if (it.joinable())
							it.join();


					//
					//auto stop = high_resolution_clock::now();
					//auto duration = duration_cast<microseconds>(stop - start);
					//std::cout << "Time taken by disc load function: " << duration.count() << " microseconds" << std::endl;

					//start = high_resolution_clock::now();

					Terrain::loadLowDetailMipStackAtLevel(level);

					//stop = high_resolution_clock::now();
					//duration = duration_cast<microseconds>(stop - start);
					//std::cout << "Time taken by mipstack load function: " << duration.count() << " microseconds" << std::endl;


					//start = high_resolution_clock::now();

					unsigned char* heightData = NULL;
					Terrain::loadTerrainChunkOnGraphicsMemoryVertical(level, heightData, -1);
					Terrain::updateHeightMapTextureArrayPartialVertical(level, heightData);
					delete[] heightData;

					//stop = high_resolution_clock::now();
					//duration = duration_cast<microseconds>(stop - start);
					//std::cout << "Time taken by gpu load function: " << duration.count() << " microseconds" << std::endl;
				}

				currentTileIndices[level].x = currentTileIndex;
				//std::cout << currentTileIndex - 2 << " " << currentTileIndex - 1 << " " << currentTileIndex << " " << currentTileIndex + 1 << std::endl;

			}
			clipmapPositions[level].x = x;

			for (int j = 0; j < 12; j++) {

				glm::vec4 startInWorldSpace;
				glm::vec4 endInWorldSpace;
				Terrain::getTerrainChunkBoundaries(blockPositions[level * 12 + j].x, blockPositions[level * 12 + j].y, level, startInWorldSpace, endInWorldSpace);
				AABB_Box aabb;
				aabb.start = startInWorldSpace;
				aabb.end = endInWorldSpace;
				blockAABBs[level * 12 + j] = aabb;
			}
		}
	}

	void Terrain::loadMapFromDiscAsync(int level, int tileIndex, int x, int z){

		tiles[level][tileIndex] = Terrain::loadHeightmapFromDisk(level, x, z);
	}

	void Terrain::deleteAllHeightData(unsigned char** heights) {

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		for (int i = 0; i < lodLevel; i++)
			delete[] heights[i];

		delete[] heights;
	}

	void Terrain::deleteAllTiles() {

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		for (int i = 0; i < lodLevel; i++) {

			int tileCount = MEM_TILE_ONE_SIDE * MEM_TILE_ONE_SIDE;
			for (int j = 0; j < tileCount; j++)
				delete[] tiles[i][j];
			delete[] tiles[i];
		}
		delete[] tiles;
	}

	void Terrain::setClipmapPositionVertical(int level, int z) {

		if (clipmapPositions[level].z != z) {

			int parcelSizeInReal = TILE_SIZE * (1 << level);
			int currentTileIndex = z / parcelSizeInReal;

			if (currentTileIndices[level].z != currentTileIndex) {

				int offset = currentTileIndex - currentTileIndices[level].z;

				if (offset == 1) {

					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++)
						delete[] tiles[level][i];

					for (int i = 0; i < MEM_TILE_ONE_SIDE - 1; i++)
						for (int j = 0; j < MEM_TILE_ONE_SIDE; j++)
							tiles[level][i * MEM_TILE_ONE_SIDE + j] = tiles[level][(i + 1) * MEM_TILE_ONE_SIDE + j];

					std::vector<std::thread> pool;

					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++) {
						int offset = MEM_TILE_ONE_SIDE / 2;
						std::thread th(&Terrain::loadMapFromDiscAsync, this, level, (MEM_TILE_ONE_SIDE - 1) * MEM_TILE_ONE_SIDE + i, currentTileIndices[level].x - offset + i, currentTileIndex + offset - 1);
						pool.push_back(std::move(th));
					}

					for (auto& it : pool)
						if (it.joinable())
							it.join();

					Terrain::loadLowDetailMipStackAtLevel(level);

					unsigned char* heightData = NULL;
					Terrain::loadTerrainChunkOnGraphicsMemoryHorizontal(level, heightData, 1);
					Terrain::updateHeightMapTextureArrayPartialHorizontal(level, heightData);
					delete[] heightData;

					toroidalUpdateIndices[level].z++;
					toroidalUpdateIndices[level].z = (toroidalUpdateIndices[level].z + (MEM_TILE_ONE_SIDE - 2)) % (MEM_TILE_ONE_SIDE - 2);
				}
				else if (offset == -1) {

					toroidalUpdateIndices[level].z--;
					toroidalUpdateIndices[level].z = (toroidalUpdateIndices[level].z + (MEM_TILE_ONE_SIDE - 2)) % (MEM_TILE_ONE_SIDE - 2);

					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++)
						delete[] tiles[level][(MEM_TILE_ONE_SIDE - 1) * MEM_TILE_ONE_SIDE + i];

					for (int i = MEM_TILE_ONE_SIDE - 1; i >= 0; i--)
						for (int j = 0; j < MEM_TILE_ONE_SIDE; j++)
							tiles[level][i * MEM_TILE_ONE_SIDE + j] = tiles[level][(i - 1) * MEM_TILE_ONE_SIDE + j];

					std::vector<std::thread> pool;
					for (int i = 0; i < MEM_TILE_ONE_SIDE; i++) {
						int offset = MEM_TILE_ONE_SIDE / 2;
						std::thread th(&Terrain::loadMapFromDiscAsync, this, level, i, currentTileIndices[level].x - offset + i, currentTileIndex - offset);
						pool.push_back(std::move(th));
					}

					for (auto& it : pool)
						if (it.joinable())
							it.join();

					Terrain::loadLowDetailMipStackAtLevel(level);

					unsigned char* heightData = NULL;
					Terrain::loadTerrainChunkOnGraphicsMemoryHorizontal(level, heightData, -1);
					Terrain::updateHeightMapTextureArrayPartialHorizontal(level, heightData);
					delete[] heightData;
				}

				currentTileIndices[level].z = currentTileIndex;
			}
			clipmapPositions[level].z = z;

			for (int j = 0; j < 12; j++) {

				glm::vec4 startInWorldSpace;
				glm::vec4 endInWorldSpace;
				Terrain::getTerrainChunkBoundaries(blockPositions[level * 12 + j].x, blockPositions[level * 12 + j].y, level, startInWorldSpace, endInWorldSpace);
				AABB_Box aabb;
				aabb.start = startInWorldSpace;
				aabb.end = endInWorldSpace;
				blockAABBs[level * 12 + j] = aabb;
			}
		}
	}

	void Terrain::setBoundariesOfClipmap(const int& level, glm::vec3& start, glm::vec3& end) {

		glm::vec2 bounds;
		Terrain::getMaxAndMinHeights(bounds, level, start, end);
		start.y = bounds.x;
		end.y = bounds.y;
	}

	void Terrain::getMaxAndMinHeights(glm::vec2& bounds, const int& level, const glm::vec3& start, const glm::vec3& end) {

		//bounds.x = 999999;
		//bounds.y = -999999;

		//int startX = (int)start.x >> level;
		//int endX = (int)end.x >> level;
		//int startZ = (int)start.z >> level;
		//int endZ = (int)end.z >> level;
		//int mipMapsize = elevationMapSize >> level;
		//for (int i = startZ; i < endZ; i++) {

		//	for (int j = startX; j < endX; j++) {

		//		float height = heightMipMaps[level][i * mipMapsize + j];
		//		if (height < bounds.x) bounds.x = height;
		//		if (height > bounds.y) bounds.y = height;
		//	}
		//}
	}
	
	char* Terrain::getNormalMap(int size) {

		char* data = new char[size * size * 3];

		for (int i = 0; i < size; i++)
			for (int j = 0; j < size; j++) {

				data[(i * size + j) * 3] = 0;
				data[(i * size + j) * 3 + 1] = 127;
				data[(i * size + j) * 3 + 2] = 0;
			}

		return data;
	}

	//float** Terrain::getFlatHeightmap() {

	//	//int mipmaps = 0;
	//	//for (int i = elevationMapSize; i > 1; i >>= 1)
	//	//	mipmaps++;

	//	//float** heightMipMaps = new float* [mipmaps];

	//	//mipmaps = 0;
	//	//for (int size = elevationMapSize; size > 1; size >>= 1) {

	//	//	float* data = new float[size * size];

	//	//	for (int i = 0; i < size; i++)
	//	//		for (int j = 0; j < size; j++)
	//	//			data[i * size + j] = 0;

	//	//	heightMipMaps[mipmaps] = data;
	//	//	mipmaps++;
	//	//}

	//	//return heightMipMaps;
	//}

	void Terrain::createDummyHeightmapTextureSet(int tileSize, int numTiles) {

		const siv::PerlinNoise perlin{ perlinSeed };
		std::vector<std::thread> pool;

		int num = numTiles * numTiles;
		for (int i = 0; i < num; i++) {

			int ind_x = i % numTiles;
			int ind_z = i / numTiles;

			int coord_x = ind_x * tileSize;
			int coord_z = ind_z * tileSize;

			std::thread th(&Terrain::createHeightmap, this, perlin, tileSize, ind_x, ind_z);
			pool.push_back(std::move(th));
		}

		for (auto& it : pool)
			if(it.joinable())
				it.join();

	}

	void Terrain::createDummyHeightmapTexture(int tileSize, int resolution) {

		int lodLevel = Terrain::getMaxMipLevel(resolution, tileSize);
		unsigned char* heights = new unsigned char[resolution * resolution * 2];
		int numTiles = resolution / tileSize;

		const siv::PerlinNoise perlin{ perlinSeed };
		std::vector<std::thread> pool;

		int num = numTiles * numTiles;
		for (int i = 0; i < num; i++) {

			int ind_x = i % numTiles;
			int ind_z = i / numTiles;

			std::thread th(&Terrain::applyPerlinNoiseOnPart, this, perlin, heights, tileSize, resolution, ind_x, ind_z);
			pool.push_back(std::move(th));
		}

		for (auto& it : pool)
			if (it.joinable())
				it.join();

		unsigned char** heightMapList = new unsigned char* [lodLevel];
		heightMapList[0] = heights;
		int res = resolution;

		for (int i = 1; i < lodLevel; i++) {

			unsigned char* newTexture = new unsigned char[(res * res) / 2];
			Terrain::createMipmapFromBase(heightMapList[i - 1], newTexture, res);
			heightMapList[i] = newTexture;
			res /= 2;
		}

		res = resolution;
		for (int level = 0; level < lodLevel; level++) {

			int numTiles = res / TILE_SIZE;
			for(int i = 0; i < numTiles; i++)
				for (int j = 0; j < numTiles; j++)
					//make multi-threaded
					Terrain::createHeightmapImageFile(heightMapList[level], level, TILE_SIZE, res, j, i);

			res /= 2;
		}
	}

	void Terrain::fucker(unsigned char* baseTexture, int baseSize) {

		const siv::PerlinNoise perlin{ perlinSeed };
		for (int z = 0; z < baseSize; z++) {
			for (int x = 0; x < baseSize; x++) {

				float height = perlin.octave2D_01((double)x * perlinScale, (double)z * perlinScale, perlinOctaves, perlinPersistence) * heightScale;
				int heightMultHundred = height * 1000;
				int r = heightMultHundred / 256;
				int g = heightMultHundred % 256;
				baseTexture[(z * baseSize + x) * 2] = r;
				baseTexture[(z * baseSize + x) * 2 + 1] = g;
			}
		}
	}

	void Terrain::createMipmapFromBase(unsigned char* baseTexture, unsigned char* newTexture, int baseSize) {

		int size = baseSize / 2;
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {

				int newTextureIndex = (i * size + j) * 2;
				int baseTextureIndex = ((i * 2) * baseSize + (j * 2)) * 2;
				newTexture[newTextureIndex] = baseTexture[baseTextureIndex];
				newTexture[newTextureIndex + 1] = baseTexture[baseTextureIndex + 1];
			}
		}
	}

	void Terrain::createHeightmap(const siv::PerlinNoise& perlin, int tileSize, int ind_x, int ind_z) {

		//const siv::PerlinNoise perlin{ perlinSeed };
		std::vector<unsigned char> heightImage(tileSize * tileSize * 2);

		int coord_x = ind_x * tileSize;
		int coord_z = ind_z * tileSize;

		for (int z = 0; z < tileSize; z++) {
			for (int x = 0; x < tileSize; x++) {

				float height = perlin.octave2D_01((double)(x + coord_x) * perlinScale, (double)(z + coord_z) * perlinScale, perlinOctaves, perlinPersistence) * heightScale;
				int heightMultHundred = height * 1000;
				int r = heightMultHundred / 256;
				int g = heightMultHundred % 256;
				heightImage[(z * tileSize + x) * 2] = r;
				heightImage[(z * tileSize + x) * 2 + 1] = g;
			}
		}

		unsigned int width = tileSize;
		unsigned int height = tileSize;
		std::string imagePath = "map_" + std::to_string(ind_z) + '_' + std::to_string(ind_x) + "_.png";
		TextureFile::encodeTextureFile(width, height, heightImage, &imagePath[0]);
	}

	void Terrain::createHeightmapImageFile(unsigned char* heights, int level, int newTileSize, int baseTileSize, int ind_x, int ind_z) {

		std::vector<unsigned char> heightImage(newTileSize * newTileSize * 2);

		int coord_x = ind_x * newTileSize;
		int coord_z = ind_z * newTileSize;

		for (int z = 0; z < newTileSize; z++) {
			for (int x = 0; x < newTileSize; x++) {

				int baseCoord = ((z + coord_z) * baseTileSize + (x + coord_x)) * 2;
				int newCoord = (z * newTileSize + x) * 2;
				heightImage[newCoord] = heights[baseCoord];
				heightImage[newCoord + 1] = heights[baseCoord + 1];
			}
		}

		unsigned int width = newTileSize;
		unsigned int height = newTileSize;
		std::string imagePath = "map_" + std::to_string(level) + '_' + std::to_string(ind_z) + '_' + std::to_string(ind_x) + "_.png";
		TextureFile::encodeTextureFile(width, height, heightImage, &imagePath[0]);
	}

	//void Terrain::createHeightmap(const siv::PerlinNoise& perlin, int tileSize, int ind_x, int ind_z) {

	//	//const siv::PerlinNoise perlin{ perlinSeed };
	//	std::vector<unsigned char> heightImage(tileSize * tileSize * 2);

	//	int coord_x = ind_x * tileSize;
	//	int coord_z = ind_z * tileSize;

	//	for (int z = 0; z < tileSize; z++) {
	//		for (int x = 0; x < tileSize; x++) {

	//			float height = perlin.octave2D_01((double)(x + coord_x) * perlinScale, (double)(z + coord_z) * perlinScale, perlinOctaves, perlinPersistence) * heightScale;
	//			int heightMultHundred = height * 1000;
	//			int r = heightMultHundred / 256;
	//			int g = heightMultHundred % 256;
	//			heightImage[(z * tileSize + x) * 2] = r;
	//			heightImage[(z * tileSize + x) * 2 + 1] = g;
	//		}
	//	}

	//	unsigned int width = tileSize;
	//	unsigned int height = tileSize;
	//	std::string imagePath = "map_" + std::to_string(ind_z) + '_' + std::to_string(ind_x) + "_.png";
	//	TextureFile::encodeTextureFile(width, height, heightImage, &imagePath[0]);
	//}

	void Terrain::applyPerlinNoiseOnPart(const siv::PerlinNoise& perlin, unsigned char* heights, int tileSize, int baseTileSize, int ind_x, int ind_z) {

		int coord_x = ind_x * tileSize;
		int coord_z = ind_z * tileSize;

		for (int z = 0; z < tileSize; z++) {
			for (int x = 0; x < tileSize; x++) {

				float height = perlin.octave2D_01((double)(x + coord_x) * perlinScale, (double)(z + coord_z) * perlinScale, perlinOctaves, perlinPersistence) * heightScale;
				int heightMultHundred = height * 1000;
				int r = heightMultHundred / 256;
				int g = heightMultHundred % 256;
				heights[((coord_z + z) * baseTileSize + coord_x + x) * 2] = r;
				heights[((coord_z + z) * baseTileSize + coord_x + x) * 2 + 1] = g;
			}
		}
	}

	void Terrain::createHeightMapWithPerlinNoise(std::vector<float>& heightImage) {

		//const siv::PerlinNoise perlin{ perlinSeed };

		//for (int z = 0; z < elevationMapSize; z++) {
		//	for (int x = 0; x < elevationMapSize; x++) {

		//		float height = perlin.octave2D((double)x * perlinScale, (double)z * perlinScale, perlinOctaves, perlinPersistence) * heightScale;
		//		heightImage[z * elevationMapSize + x] = 0;// height;
		//	}
		//}

		int tileSize = 256;

		std::vector<unsigned char> out;
		unsigned w;
		unsigned h;
		lodepng::decode(out, w, h, "map_0_0_.png", LodePNGColorType::LCT_GREY, 16);

		const siv::PerlinNoise perlin{ perlinSeed };

		for (int z = 0; z < tileSize; z++) {
			for (int x = 0; x < tileSize; x++) {

				float height = (out[(z * tileSize + x) * 2] * 256 + out[(z * tileSize + x) * 2 + 1]) / 1000.f;
				heightImage[z * tileSize + x] = height;
			}
		}
	}

	// not tested.
	Vec2Int Terrain::getClipmapPosition(int level, glm::vec3& camPos) {

		int patchWidth = 2;
		float patchOffset = 1 << level;
		float requiredCameraDisplacement = patchWidth * patchOffset;
		float posX = (int)(camPos.x / requiredCameraDisplacement) * requiredCameraDisplacement;
		float posZ = (int)(camPos.z / requiredCameraDisplacement) * requiredCameraDisplacement;
		return Vec2Int(posX + patchOffset, posZ + patchOffset);
	}

	void Terrain::recalculateNormals(std::vector<float>& heightImage, std::vector<char>& normalImage) {

		//for (int z = 1; z < elevationMapSize - 1; z++) {
		//	for (int x = 1; x < elevationMapSize - 1; x++) {

		//		// easy way
		//		int index0 = (z - 1) * elevationMapSize + x;
		//		int index1 = z * elevationMapSize + x - 1;
		//		int index2 = z * elevationMapSize + x + 1;
		//		int index3 = (z + 1) * elevationMapSize + x;

		//		float h0 = heightImage[index0];
		//		float h1 = heightImage[index1];
		//		float h2 = heightImage[index2];
		//		float h3 = heightImage[index3];

		//		glm::vec3 normal;
		//		normal.z = h0 - h3;
		//		normal.x = h1 - h2;
		//		normal.y = 2;

		//		normal = glm::normalize(normal);

		//		normalImage[(z * elevationMapSize + x) * 3] = (char)(normal.x * 127);
		//		normalImage[(z * elevationMapSize + x) * 3 + 1] = (char)(normal.y * 127);
		//		normalImage[(z * elevationMapSize + x) * 3 + 2] = (char)(normal.z * 127);
		//	}
		//}

		

		//// sobel
		//float tl = heights[(z - 1) * elevationMapSize + (x - 1)]; // top left
		//float l  = heights[z * elevationMapSize + (x - 1)]; // left
		//float bl = heights[(z + 1) * elevationMapSize + (x - 1)]; // bottom left
		//float t  = heights[(z - 1) * elevationMapSize + x]; // top
		//float b  = heights[(z + 1) * elevationMapSize + x]; // bottom
		//float tr = heights[(z - 1) * elevationMapSize + (x + 1)]; // top right
		//float r  = heights[z * elevationMapSize + (x + 1)]; // right
		//float br = heights[(z + 1) * elevationMapSize + (x + 1)]; // bottom right

		//float dX = tr + 2 * r + br - tl - 2 * l - bl;   
		//float dY = bl + 2 * b + br - tl - 2 * t - tr;

		//float normalStrength = 1.f;
		//glm::vec3 N = glm::normalize(glm::vec3(dX, 2.0 / normalStrength, dY));

		//normals[(z * elevationMapSize + x) * 3] = (char)(N.x * 127);
		//normals[(z * elevationMapSize + x) * 3 + 1] = (char)(N.y * 127);
		//normals[(z * elevationMapSize + x) * 3 + 2] = (char)(N.z * 127);
	}

	void Terrain::streamFromTexture(int level, int oldCamPosX, int oldCamPosZ, int newCamPosX, int newCamPosZ) {

	}
	void Terrain::changeCurrentTileIndex_X(int level, int currentIndex) {

		currentTileIndices[level].x = currentIndex;

		if(level == 0)
			std::cout << currentIndex - 1 << " " << currentIndex << " " << currentIndex + 1 << std::endl;
	}

	void Terrain::updateStreamFrom_X(int level, int newCamPosX) {

		int wholeClipmapRegionSize = clipmapResolution * 4 * (1 << level);

		//int startX = clipmapPositions[level].x;

		//clipmapPositions[level].x = newCamPosX;
	}

	int Terrain::getMaxMipLevel(int textureSize, int tileSize) {

		int lodLevel = 0;
		int num = tileSize;
		while (num <= textureSize) {
			num *= 2;
			lodLevel++;
		}
		return lodLevel;
	}

	glm::vec2 Terrain::getMinAndMaxPointOfTerrainChunk(int startX, int startZ, int endX, int endZ, int level) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		int divisionMultiplier = 8;
		int texSize = TILE_SIZE * gpuParcels;
		int mipmapSize = texSize / divisionMultiplier;

		float min = 999999999.f;
		float max = -999999999.f;

		for (int i = startZ; i < endZ; i++) {
			for (int j = startX; j < endX; j++) {

				int index = (i * mipmapSize + j) * 2;
				float height = (lowDetailMipStack[level][index] * 256 + lowDetailMipStack[level][index + 1]) * 0.001f;

				if (height < min)
					min = height;

				if (height > max)
					max = height;
			}
		}

		return glm::vec2(min, max);
	}

	void Terrain::getTerrainChunkBoundaries(int blockPosX, int blockPosZ, int level, glm::vec4& startInWorldSpace, glm::vec4& endInWorldSpace) {

		int gpuParcels = MEM_TILE_ONE_SIDE - 2;
		int divisionMultiplier = 8;
		int textureSizeInWorldSpace = TILE_SIZE * gpuParcels * (1 << level);
		int texSize = TILE_SIZE * gpuParcels;
		int mipmapSize = texSize / divisionMultiplier;

		int texturePosX = currentTileIndices[level].x * TILE_SIZE * (1 << level);
		int texturePosZ = currentTileIndices[level].z * TILE_SIZE * (1 << level);

		int startX = blockPosX - (texturePosX - textureSizeInWorldSpace / 2);
		int startZ = blockPosZ - (texturePosZ - textureSizeInWorldSpace / 2);
		int endX = startX + clipmapResolution * (1 << level);
		int endZ = startZ + clipmapResolution * (1 << level);
		
		int startXinMipmap = (startX * mipmapSize) / textureSizeInWorldSpace;
		int startZinMipmap = (startZ * mipmapSize) / textureSizeInWorldSpace;
		int endXinMipmap = (endX * mipmapSize) / textureSizeInWorldSpace;
		int endZinMipmap = (endZ * mipmapSize) / textureSizeInWorldSpace;

		glm::vec2 localPoints = Terrain::getMinAndMaxPointOfTerrainChunk(startXinMipmap, startZinMipmap, endXinMipmap, endZinMipmap, level);
		startInWorldSpace = glm::vec4(blockPosX, localPoints.x, blockPosZ, 1);
		endInWorldSpace = glm::vec4(blockPosX + clipmapResolution * (1 << level), localPoints.y, blockPosZ + clipmapResolution * (1 << level), 1);
	}

}