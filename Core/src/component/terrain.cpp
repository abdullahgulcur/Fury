#include "pch.h"
#include "terrain.h"
//#include "entity.h"
#include "core.h"
#include "scenemanager.h"
#include "scene.h"
#include "glewcontext.h"
#include "gl/glew.h"
#include "component/gamecamera.h"
#include "lodepng/lodepng.h"
#include "FreeImage.h"

using namespace std::chrono;

namespace Fury {

	Terrain::Terrain(Entity* entity) : Component(entity) { }

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

	void Terrain::start() {


		GameCamera* camera = Core::instance->sceneManager->currentScene->primaryCamera;
		//glm::vec3 camPos = camera->entity->transform->getGlobalPosition();

		//if (camPos.x < 4096.f || camPos.x > 6144.f || camPos.z < 4096.f || camPos.z > 6144.f) {
		//	camera->position.x = 5000.f;
		//	camera->position.z = 5000.f;
		//}

		GlewContext* glew = Core::instance->glewContext;
		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);


		//// create mipmaps
		//std::string path = "terrain.png";
		//std::vector<unsigned char> out;
		//unsigned int w, h;
		//lodepng::decode(out, w, h, path, LodePNGColorType::LCT_GREY, 16);
		//unsigned char* data = new unsigned char[w * w * 2];
		//for (int i = 0; i < out.size(); i++)
		//	data[i] = out[i];

		//unsigned char** heightMapList = Terrain::createMipmaps(data, w);
		//Terrain::divideTerrainHeightmaps(heightMapList, w);


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
		glew->uniform1i(glew->getUniformLocation(programID, "albedoArray"), 4);
		glew->uniform1i(glew->getUniformLocation(programID, "normalArray"), 5);
		glew->uniform1i(glew->getUniformLocation(programID, "maskArray"), 6);

		cameraPosition = camera->position;
		Terrain::loadTerrainHeightmapOnInit(cameraPosition, lodLevel);
		Terrain::calculateBlockPositions(cameraPosition, lodLevel);

		for (int i = 0; i < lodLevel; i++)
			for (int j = 0; j < 12; j++)
				blockAABBs[12 * i + j] = Terrain::getBoundingBoxOfClipmap(j, i);

		Terrain::createAlbedoMapTextureArray();
	}

	void Terrain::loadTerrainHeightmapOnInit(glm::vec3 camPos, int clipmapLevel) {

		for (int level = 0; level < clipmapLevel; level++)
			Terrain::loadHeightmapAtLevel(level, camPos, heights[level]);

		Terrain::createElevationMapTextureArray(heights);
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
		//std::vector<unsigned char> out(TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2);
		//for (int i = 0; i < TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2; i++)
		//	out[i] = heights[level][i];

		//unsigned int width = TILE_SIZE * MEM_TILE_ONE_SIDE;
		//unsigned int height = TILE_SIZE * MEM_TILE_ONE_SIDE;
		//std::string imagePath = "heights" + std::to_string(level) + ".png";
		//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
	}

	void Terrain::update(float dt) {

		int level = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		glm::vec3 camPosition = Core::instance->sceneManager->currentScene->primaryCamera->position;
		camPosition = glm::clamp(camPosition, glm::vec3(4100,0,4100), glm::vec3(6100,0,6100));

		Terrain::calculateBlockPositions(camPosition, level);
		Terrain::streamTerrain(camPosition, level);
	}

	void Terrain::onDraw(glm::mat4& pv, glm::vec3& pos) {

		Scene* scene = Core::instance->sceneManager->currentScene;
		GameCamera* camera = scene->primaryCamera;
		GlewContext* glew = Core::instance->glewContext;

		int level = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		int programID = this->programID;

		float* lightDirAddr = &glm::normalize(-glm::vec3(0.5, -1, 0.5))[0];

		glm::vec3 test = camera->position;

		int blockVAO = this->blockVAO;
		int ringFixUpVAO = this->ringFixUpVAO;
		int smallSquareVAO = this->smallSquareVAO;
		int interiorTrimVAO = this->interiorTrimVAO;
		int outerDegenerateVAO = this->outerDegenerateVAO;

		int blockIndiceCount = blockIndices.size();
		int ringFixUpIndiceCount = ringFixUpIndices.size();
		int smallSquareIndiceCount = smallSquareIndices.size();
		int interiorTrimIndiceCount = interiorTrimIndices.size();
		int outerDegenerateIndiceCount = outerDegenerateIndices.size();

		GlobalVolume* globalVolume = Core::instance->fileSystem->globalVolume;

		glew->useProgram(programID);
		glew->uniformMatrix4fv(glew->getUniformLocation(programID, "PV"), 1, 0, &pv[0][0]);
		glew->uniform3fv(glew->getUniformLocation(programID, "camPos"), 1, &pos[0]);
		glew->uniform3fv(glew->getUniformLocation(programID, "camPoss"), 1, &test[0]);
		glew->uniform1f(glew->getUniformLocation(programID, "texSize"), (float)TILE_SIZE * MEM_TILE_ONE_SIDE);
		glew->uniform1f(glew->getUniformLocation(programID, "heightScale"), -displacementMapScale);

		// bind pre-computed IBL data
		glew->activeTexture(GL_TEXTURE0);
		glew->bindTexture(GL_TEXTURE_CUBE_MAP, globalVolume->irradianceMap);
		glew->activeTexture(GL_TEXTURE1);
		glew->bindTexture(GL_TEXTURE_CUBE_MAP, globalVolume->prefilterMap);
		glew->activeTexture(GL_TEXTURE2);
		glew->bindTexture(GL_TEXTURE_2D, globalVolume->brdfLUTTexture);

		glew->activeTexture(GL_TEXTURE3);
		glew->bindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTexture);

		glew->activeTexture(GL_TEXTURE4);
		glew->bindTexture(GL_TEXTURE_2D_ARRAY, albedoTextureArray);
		glew->activeTexture(GL_TEXTURE5);
		glew->bindTexture(GL_TEXTURE_2D_ARRAY, normalTextureArray);
		glew->activeTexture(GL_TEXTURE6);
		glew->bindTexture(GL_TEXTURE_2D_ARRAY, maskTextureArray);
		
		glm::vec2* blockPositions = this->blockPositions;
		glm::vec2* ringFixUpPositions = this->ringFixUpPositions;
		glm::vec2* interiorTrimPositions = this->interiorTrimPositions;
		glm::vec2* outerDegeneratePositions = this->outerDegeneratePositions;
		float* rotAmounts = this->rotAmounts;
		glm::vec2 smallSquarePosition = this->smallSquarePosition;

		std::vector<TerrainVertexAttribs> instanceArray;

		// BLOCKS

		for (int i = 0; i < level; i++) {

			for (int j = 0; j < 12; j++) {

				glm::vec4 startInWorldSpace;
				glm::vec4 endInWorldSpace;
				AABB_Box aabb = blockAABBs[i * 12 + j];
				startInWorldSpace = aabb.start;
				endInWorldSpace = aabb.end;
			//	if (camera->intersectsAABB(startInWorldSpace, endInWorldSpace)) {

					TerrainVertexAttribs attribs;
					attribs.level = i;
					attribs.model = glm::mat4(1);
					attribs.position = glm::vec2(blockPositions[i * 12 + j].x, blockPositions[i * 12 + j].y);
					glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(i, test);
					attribs.clipmapcenter = clipmapcenter;
					instanceArray.push_back(attribs);
			//	}
			}
		}

		for (int i = 0; i < 4; i++) {

			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			attribs.position = glm::vec2(blockPositions[level * 12 + i].x, blockPositions[level * 12 + i].y);
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(0, test);
			attribs.clipmapcenter = clipmapcenter;
			instanceArray.push_back(attribs);
		}

		int size = sizeof(TerrainVertexAttribs);
		Terrain::drawElementsInstanced(glew, size, blockVAO, instanceArray, blockIndiceCount);
		instanceArray.clear();

		// RING FIXUP
		for (int i = 0; i < level; i++) {

			glm::mat4 model = glm::mat4(1);

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = model;
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(i, test);
			attribs.clipmapcenter = clipmapcenter;

			attribs.position = glm::vec2(ringFixUpPositions[i * 4 + 0].x, ringFixUpPositions[i * 4 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[i * 4 + 2].x, ringFixUpPositions[i * 4 + 2].y);
			instanceArray.push_back(attribs);

			model = glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
			attribs.model = model;

			attribs.position = glm::vec2(ringFixUpPositions[i * 4 + 1].x, ringFixUpPositions[i * 4 + 1].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[i * 4 + 3].x, ringFixUpPositions[i * 4 + 3].y);
			instanceArray.push_back(attribs);
		}

		{

			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(0, test);
			attribs.clipmapcenter = clipmapcenter;

			attribs.position = glm::vec2(ringFixUpPositions[level * 4 + 0].x, ringFixUpPositions[level * 4 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[level * 4 + 2].x, ringFixUpPositions[level * 4 + 2].y);
			instanceArray.push_back(attribs);

			glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
			attribs.model = model;

			attribs.position = glm::vec2(ringFixUpPositions[level * 4 + 1].x, ringFixUpPositions[level * 4 + 1].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[level * 4 + 3].x, ringFixUpPositions[level * 4 + 3].y);
			instanceArray.push_back(attribs);
		}

		Terrain::drawElementsInstanced(glew, size, ringFixUpVAO, instanceArray, ringFixUpIndiceCount);
		instanceArray.clear();

		// INTERIOR TRIM
		for (int i = 0; i < level - 1; i++) {

			glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(rotAmounts[i]), glm::vec3(0.0f, 1.0f, 0.0f));

			TerrainVertexAttribs attribs;
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(i + 1, test);
			attribs.clipmapcenter = clipmapcenter;
			attribs.level = i + 1;
			attribs.model = model;
			attribs.position = glm::vec2(interiorTrimPositions[i].x, interiorTrimPositions[i].y);
			instanceArray.push_back(attribs);
		}
		Terrain::drawElementsInstanced(glew, size, interiorTrimVAO, instanceArray, interiorTrimIndiceCount);
		instanceArray.clear();

		// OUTER DEGENERATE
		for (int i = 0; i < level; i++) {

			glm::mat4 model = glm::mat4(1);

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = model;
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(i, test);
			attribs.clipmapcenter = clipmapcenter;

			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 0].x, outerDegeneratePositions[i * 4 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 1].x, outerDegeneratePositions[i * 4 + 1].y);
			instanceArray.push_back(attribs);

			model = glm::rotate(glm::mat4(1), glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f));
			attribs.model = model;

			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 2].x, outerDegeneratePositions[i * 4 + 2].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 3].x, outerDegeneratePositions[i * 4 + 3].y);
			instanceArray.push_back(attribs);
		}
		Terrain::drawElementsInstanced(glew, size, outerDegenerateVAO, instanceArray, outerDegenerateIndiceCount);
		instanceArray.clear();

		// SMALL SQUARE
		TerrainVertexAttribs attribs;
		glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(0, test);
		attribs.clipmapcenter = clipmapcenter;
		attribs.level = 0;
		attribs.model = glm::mat4(1);
		attribs.position = glm::vec2(smallSquarePosition.x, smallSquarePosition.y);
		instanceArray.push_back(attribs);
		Terrain::drawElementsInstanced(glew, size, smallSquareVAO, instanceArray, smallSquareIndiceCount);
	}

	void Terrain::drawElementsInstanced(GlewContext* glew, int size, unsigned int VAO, std::vector<TerrainVertexAttribs>& instanceArray, unsigned int indiceCount) {

		unsigned int instanceBuffer;
		glew->genBuffers(1, &instanceBuffer);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->bufferData(0x8892, instanceArray.size() * sizeof(TerrainVertexAttribs), &instanceArray[0], 0x88E4);

		glew->bindVertexArray(VAO);

		glew->enableVertexAttribArray(1);
		glew->vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, size, (void*)0);
		glew->enableVertexAttribArray(2);
		glew->vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2)));
		glew->enableVertexAttribArray(3);
		glew->vertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2));
		glew->enableVertexAttribArray(4);
		glew->vertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2 + sizeof(float)));
		glew->enableVertexAttribArray(5);
		glew->vertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2 + sizeof(float) + sizeof(glm::vec4)));
		glew->enableVertexAttribArray(6);
		glew->vertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2 + sizeof(float) + sizeof(glm::vec4) * 2));
		glew->enableVertexAttribArray(7);
		glew->vertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2 + sizeof(float) + sizeof(glm::vec4) * 3));

		glew->vertexAttribDivisor(1, 1);
		glew->vertexAttribDivisor(2, 1);
		glew->vertexAttribDivisor(3, 1);
		glew->vertexAttribDivisor(4, 1);
		glew->vertexAttribDivisor(5, 1);
		glew->vertexAttribDivisor(6, 1);

		glew->drawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(indiceCount), GL_UNSIGNED_INT, 0, instanceArray.size());

		glew->bindVertexArray(0);
		glew->deleteBuffers(1, &instanceBuffer);
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

				old_border.x++;
				old_border.x %= MEM_TILE_ONE_SIDE;
				old_tileStart.x++;
			}

			delete[] heightData;


			//// DEBUG
			//std::vector<unsigned char> out(TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2);
			//for (int i = 0; i < TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2; i++)
			//	out[i] = heights[level][i];

			//unsigned int width = TILE_SIZE * MEM_TILE_ONE_SIDE;
			//unsigned int height = TILE_SIZE * MEM_TILE_ONE_SIDE;
			//std::string imagePath = "heights" + std::to_string(level) + ".png";
			//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
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

				old_border.y++;
				old_border.y %= MEM_TILE_ONE_SIDE;
				old_tileStart.y++;
			}

			delete[] heightData;

			//// DEBUG
			//std::vector<unsigned char> out(TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2);
			//for (int i = 0; i < TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2; i++)
			//	out[i] = heights[level][i];

			//unsigned int width = TILE_SIZE * MEM_TILE_ONE_SIDE;
			//unsigned int height = TILE_SIZE * MEM_TILE_ONE_SIDE;
			//std::string imagePath = "heights" + std::to_string(level) + ".png";
			//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
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
			
				old_tileStart.y--;
			}

			delete[] heightData;
		}
	}

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

	void Terrain::createElevationMapTextureArray(unsigned char** heightmapArray) {

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

	void Terrain::createAlbedoMapTextureArray() {

		std::string albedoTexturePaths[8];
		std::string normalTexturePaths[8];
		std::string roughnessTexturePaths[8];
		std::string aoTexturePaths[8];
		std::string heightTexturePaths[8];

		//unsigned int width = 512;
		//unsigned int height = 512;
		//unsigned int size = width * height * 4;

		unsigned char* albedoMaps = new unsigned char[512 * 512 * 4 * 8];
		unsigned char* normalMaps = new unsigned char[512 * 512 * 4 * 8];
		unsigned char* maskMaps = new unsigned char[512 * 512 * 4 * 8];

		albedoTexturePaths[0] = "temp/moss_0_a.png";
		albedoTexturePaths[1] = "temp/moss_2_a.png";
		albedoTexturePaths[2] = "temp/mud_wet_a.png";
		albedoTexturePaths[3] = "temp/mud_cracked_a.png";
		albedoTexturePaths[4] = "temp/sand_muddy_a.png";
		albedoTexturePaths[5] = "temp/soil_rocky_a.png";
		albedoTexturePaths[6] = "temp/cliff_volcanic_a.png";
		albedoTexturePaths[7] = "temp/cliff_granite_a.png";

		normalTexturePaths[0] = "temp/moss_0_n.png";
		normalTexturePaths[1] = "temp/moss_2_n.png";
		normalTexturePaths[2] = "temp/mud_wet_n.png";
		normalTexturePaths[3] = "temp/mud_cracked_n.png";
		normalTexturePaths[4] = "temp/sand_muddy_n.png";
		normalTexturePaths[5] = "temp/soil_rocky_n.png";
		normalTexturePaths[6] = "temp/cliff_volcanic_n.png";
		normalTexturePaths[7] = "temp/cliff_granite_n.png";

		roughnessTexturePaths[0] = "temp/moss_0_r.png";
		roughnessTexturePaths[1] = "temp/moss_2_r.png";
		roughnessTexturePaths[2] = "temp/mud_wet_r.png";
		roughnessTexturePaths[3] = "temp/mud_cracked_r.png";
		roughnessTexturePaths[4] = "temp/sand_muddy_r.png";
		roughnessTexturePaths[5] = "temp/soil_rocky_r.png";
		roughnessTexturePaths[6] = "temp/cliff_volcanic_r.png";
		roughnessTexturePaths[7] = "temp/cliff_granite_r.png";

		aoTexturePaths[0] = "temp/moss_0_ao.png";
		aoTexturePaths[1] = "temp/moss_2_ao.png";
		aoTexturePaths[2] = "temp/mud_wet_ao.png";
		aoTexturePaths[3] = "temp/mud_cracked_ao.png";
		aoTexturePaths[4] = "temp/sand_muddy_ao.png";
		aoTexturePaths[5] = "temp/soil_rocky_ao.png";
		aoTexturePaths[6] = "temp/cliff_volcanic_ao.png";
		aoTexturePaths[7] = "temp/cliff_granite_ao.png";

		heightTexturePaths[0] = "temp/moss_0_h.png";
		heightTexturePaths[1] = "temp/moss_2_h.png";
		heightTexturePaths[2] = "temp/mud_wet_h.png";
		heightTexturePaths[3] = "temp/mud_cracked_h.png";
		heightTexturePaths[4] = "temp/sand_muddy_h.png";
		heightTexturePaths[5] = "temp/soil_rocky_h.png";
		heightTexturePaths[6] = "temp/cliff_volcanic_h.png";
		heightTexturePaths[7] = "temp/cliff_granite_h.png";

		for (int i = 0; i < 8; i++) {

			unsigned int width, height;
			std::vector<unsigned char> out;
			lodepng::decode(out, width, height, albedoTexturePaths[i], LodePNGColorType::LCT_RGBA, 8);
			unsigned int size = width * height * 4;

			for (int j = 0; j < out.size(); j++)
				albedoMaps[size * i + j] = out[j];
		}

		//for (int i = 0; i < 8; i++) {

		//	for (int j = 0; j < 512 * 512 * 4; j++) {

		//		unsigned char test = albedoMaps[i][j];
		//		int a = 5;
		//	}
		//}

		for (int i = 0; i < 8; i++) {

			unsigned int width, height;
			std::vector<unsigned char> out;
			lodepng::decode(out, width, height, normalTexturePaths[i], LodePNGColorType::LCT_RGBA, 8);
			unsigned int size = width * height * 4;

			for (int j = 0; j < out.size(); j++)
				normalMaps[size * i + j] = out[j];

			/*unsigned int width, height;
			std::vector<unsigned char> out;
			lodepng::decode(out, width, height, normalTexturePaths[i], LodePNGColorType::LCT_RGBA, 8);

			unsigned char* data = new unsigned char[width * height * 4];
			for (int j = 0; j < out.size(); j++)
				data[j] = out[j];

			normalMaps[i] = data;*/
		}

		for (int i = 0; i < 8; i++) {

			unsigned int width, height;
			std::vector<unsigned char> out_r;
			lodepng::decode(out_r, width, height, roughnessTexturePaths[i], LodePNGColorType::LCT_RGBA, 8);
			std::vector<unsigned char> out_ao;
			lodepng::decode(out_ao, width, height, aoTexturePaths[i], LodePNGColorType::LCT_RGBA, 8);
			std::vector<unsigned char> out_h;
			lodepng::decode(out_h, width, height, heightTexturePaths[i], LodePNGColorType::LCT_RGBA, 8);

			unsigned int size = width * height;
			unsigned int arrSize = width * height * 4;

			for (int j = 0; j < size; j++) {
				maskMaps[i * arrSize + j * 4] = 0;// metalness;
				maskMaps[i * arrSize + j * 4 + 1] = out_r[j * 4];
				maskMaps[i * arrSize + j * 4 + 2] = out_ao[j * 4];
				maskMaps[i * arrSize + j * 4 + 3] = out_h[j * 4];
			}
		}

		unsigned int stride = 512 * 512 * 4;

		glGenTextures(1, &albedoTextureArray);
		glBindTexture(GL_TEXTURE_2D_ARRAY, albedoTextureArray);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 9, GL_RGBA8, 512, 512, 8);
		for (int i = 0; i < 8; i++)
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, 512, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, &albedoMaps[i * stride]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glGenTextures(1, &normalTextureArray);
		glBindTexture(GL_TEXTURE_2D_ARRAY, normalTextureArray);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 9, GL_RGBA8, 512, 512, 8);
		for (int i = 0; i < 8; i++)
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, 512, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, &normalMaps[i * stride]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glGenTextures(1, &maskTextureArray);
		glBindTexture(GL_TEXTURE_2D_ARRAY, maskTextureArray);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 9, GL_RGBA8, 512, 512, 8);
		for (int i = 0; i < 8; i++)
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, 512, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, &maskMaps[i * stride]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		delete[] albedoMaps;
		delete[] normalMaps;
		delete[] maskMaps;
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
				int indexInHeights = ((i + startZinHeights) * TILE_SIZE * MEM_TILE_ONE_SIDE + j + startXinHeights) * 2;

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

		std::string path = "heightmaps/map_" + std::to_string(level) + '_' + std::to_string(index.y) + '_' + std::to_string(index.x) + "_.png";
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
		return 4;
		//return lodLevel;
	}

	unsigned char** Terrain::createMipmaps(unsigned char* heights, int size) {

		// 8 bits
		//int mipCount = 0;
		//int sizeIterator = size;

		//while (sizeIterator >= 256) {
		//	sizeIterator /= 2;
		//	mipCount++;
		//}
		//unsigned char** mipmaps = new unsigned char* [mipCount];

		//int totalSize = size * size;
		//mipmaps[0] = new unsigned char[totalSize];

		//for (int i = 0; i < totalSize; i++)
		//	mipmaps[0][i] = heights[i];

		//int counter = 1;
		//while (counter < mipCount) {

		//	size /= 2;
		//	mipmaps[counter] = new unsigned char[size * size];

		//	for (int i = 0; i < size; i++) {
		//		for (int j = 0; j < size; j++) {

		//			int indexInFinerLevel = ((i * 2) * size * 2) + j * 2;//(i * size * 2 + j) * 2;
		//			int indexInCoarserLevel = (i * size + j);
		//			mipmaps[counter][indexInCoarserLevel] = mipmaps[counter - 1][indexInFinerLevel];
		//		}
		//	}

		//	std::vector<unsigned char> out(size * size);
		//	for (int i = 0; i < size * size; i++)
		//		out[i] = mipmaps[counter][i];

		//	unsigned int width = size;
		//	unsigned int height = size;
		//	std::string imagePath = "mipmap" + std::to_string(counter) + ".png";
		//	TextureFile::encodeTextureFile8Bits(width, height, out, &imagePath[0]);

		//	counter++;
		//}

		// 16 bits
		int mipCount = 0;
		int sizeIterator = size;

		while (sizeIterator >= TILE_SIZE) {
			sizeIterator /= 2;
			mipCount++;
		}
		unsigned char** mipmaps = new unsigned char* [mipCount];

		int totalSize = size * size * 2;
		mipmaps[0] = new unsigned char[totalSize];

		for (int i = 0; i < totalSize; i++)
			mipmaps[0][i] = heights[i];

		int counter = 1;
		while (counter < mipCount) {

			size /= 2;
			mipmaps[counter] = new unsigned char[size * size * 2];

			for (int i = 0; i < size; i++) {
				for (int j = 0; j < size; j++) {

					int indexInFinerLevel = (i * size * 2 + j) * 4;
					int indexInCoarserLevel = (i * size + j) * 2;
					mipmaps[counter][indexInCoarserLevel] = mipmaps[counter - 1][indexInFinerLevel];
					mipmaps[counter][indexInCoarserLevel + 1] = mipmaps[counter - 1][indexInFinerLevel + 1];
				}
			}

			//std::vector<unsigned char> out(size * size * 2);
			//for (int i = 0; i < size * size * 2; i++)
			//	out[i] = mipmaps[counter][i];

			//unsigned int width = size;
			//unsigned int height = size;
			//std::string imagePath = "mipmap" + std::to_string(counter) + ".png";
			//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);

			counter++;
		}

		return mipmaps;
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
		std::string imagePath = "heightmaps/map_" + std::to_string(level) + '_' + std::to_string(ind_z) + '_' + std::to_string(ind_x) + "_.png";
		TextureFile::encodeTextureFile(width, height, heightImage, &imagePath[0]);

		//std::vector<unsigned char> heightImage(newTileSize * newTileSize);

		//int coord_x = ind_x * newTileSize;
		//int coord_z = ind_z * newTileSize;

		//for (int z = 0; z < newTileSize; z++) {
		//	for (int x = 0; x < newTileSize; x++) {

		//		int baseCoord = ((z + coord_z) * baseTileSize + (x + coord_x));
		//		int newCoord = (z * newTileSize + x);
		//		heightImage[newCoord] = heights[baseCoord];
		//	}
		//}

		//unsigned int width = newTileSize;
		//unsigned int height = newTileSize;
		//std::string imagePath = "heightmaps/map_" + std::to_string(level) + '_' + std::to_string(ind_z) + '_' + std::to_string(ind_x) + "_.png";
		//TextureFile::encodeTextureFile8Bits(width, height, heightImage, &imagePath[0]);
	}

	void Terrain::divideTerrainHeightmaps(unsigned char** heightMapList, int width) {

		int mipCount = 0;
		int sizeIterator = width;

		while (sizeIterator >= TILE_SIZE) {
			sizeIterator /= 2;
			mipCount++;
		}

		int res = width;

		for (int level = 0; level < mipCount; level++) {

			int numTiles = res / TILE_SIZE;
			for (int i = 0; i < numTiles; i++)
				for (int j = 0; j < numTiles; j++)
					//make multi-threaded ?
					Terrain::createHeightmapImageFile(heightMapList[level], level, TILE_SIZE, res, j, i);

			res /= 2;
		}
	}

}