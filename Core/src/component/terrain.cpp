#include "pch.h"
#include "terrain.h"
#include "core.h"
#include "scenemanager.h"
#include "glewcontext.h"

namespace Fury {

	Terrain::Terrain() {


	}

	void Terrain::init() {

		Terrain::createHeightMap();
		Terrain::createNormalMap();
		Terrain::generateTerrainClipmapsVertexArrays();
	}

	void Terrain::createHeightMap() {

		GlewContext* glew = Core::instance->glewContext;
		glew->genTextures(1, &elevationMapTexture);
		glew->bindTexture(0x0DE1, elevationMapTexture);
		glew->texParameteri(0x0DE1, 0x2802, 0x2901);
		glew->texParameteri(0x0DE1, 0x2803, 0x2901);
		glew->texParameteri(0x0DE1, 0x2800, 0x2601);
		glew->texParameteri(0x0DE1, 0x2801, 0x2601);

		// load image, create texture and generate mipmaps
		//heights = TerrainGenerator::getFlatHeightmap(elevationMapSize);
		heightMipMaps = Terrain::getFlatHeightmap();

		//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, elevationMapSize, elevationMapSize, 0, GL_RED, GL_FLOAT, heights);
		glew->texImage2D(0x0DE1, 0, 0x822E, elevationMapSize, elevationMapSize, 0, 0x1903, 0x1406, heightMipMaps[0]);
		glew->generateMipmap(0x0DE1);
		//delete[] data;

		programID = glew->loadShaders("C:/Projects/Fury/Core/src/shader/clipmap.vert",
			"C:/Projects/Fury/Core/src/shader/clipmap.frag");
	}

	void Terrain::createNormalMap() {

		GlewContext* glew = Core::instance->glewContext;
		glew->genTextures(1, &normalMapTexture);
		glew->bindTexture(0x0DE1, normalMapTexture);
		glew->texParameteri(0x0DE1, 0x2802, 0x2901);
		glew->texParameteri(0x0DE1, 0x2803, 0x2901);
		glew->texParameteri(0x0DE1, 0x2800, 0x2601);
		glew->texParameteri(0x0DE1, 0x2801, 0x2601);
		normals = Terrain::getNormalMap(elevationMapSize);
		glew->texImage2D(0x0DE1, 0, 0x1907, elevationMapSize, elevationMapSize, 0, 0x1907, 0x1400, normals);
		glew->generateMipmap(0x0DE1);
	}

	void Terrain::generateTerrainClipmapsVertexArrays() {

		GlewContext* glew = Core::instance->glewContext;
		std::vector<glm::vec3> verts;
		std::vector<glm::vec3> ringFixUpVerts;
		std::vector<glm::vec3> ringFixUpVerts1;
		std::vector<glm::vec3> smallSquareVerts;
		std::vector<glm::vec3> outerDegenerateVerts;

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

				ringFixUpHorizontalIndices.push_back(j + i * (3));
				ringFixUpHorizontalIndices.push_back(j + (i + 1) * (3));
				ringFixUpHorizontalIndices.push_back(j + i * (3) + 1);

				ringFixUpHorizontalIndices.push_back(j + i * (3) + 1);
				ringFixUpHorizontalIndices.push_back(j + (i + 1) * (3));
				ringFixUpHorizontalIndices.push_back(j + (i + 1) * (3) + 1);
			}
		}

		for (int i = 0; i < 3; i++)
			for (int j = 0; j < clipmapResolution; j++)
				ringFixUpVerts1.push_back(glm::vec3(j, 0, i));

		for (int i = 0; i < 3 - 1; i++) {

			for (int j = 0; j < clipmapResolution - 1; j++) {

				ringFixUpVerticalIndices.push_back(j + i * (clipmapResolution));
				ringFixUpVerticalIndices.push_back(j + (i + 1) * (clipmapResolution));
				ringFixUpVerticalIndices.push_back(j + i * (clipmapResolution)+1);

				ringFixUpVerticalIndices.push_back(j + i * (clipmapResolution)+1);
				ringFixUpVerticalIndices.push_back(j + (i + 1) * (clipmapResolution));
				ringFixUpVerticalIndices.push_back(j + (i + 1) * (clipmapResolution)+1);
			}
		}

		// Outer degenerate triangles (y lere hic gerek yok !!!)
		for (int i = 0; i < clipmapResolution * 4; i += 2) {

			outerDegenerateVerts.push_back(glm::vec3(0, 0, i));
			outerDegenerateVerts.push_back(glm::vec3(0, 0, i + 1));
			outerDegenerateVerts.push_back(glm::vec3(0, 0, i + 2));
		}

		for (int i = 0; i < clipmapResolution * 4; i += 2) {

			outerDegenerateVerts.push_back(glm::vec3(i, 0, clipmapResolution * 4));
			outerDegenerateVerts.push_back(glm::vec3(i + 1, 0, clipmapResolution * 4));
			outerDegenerateVerts.push_back(glm::vec3(i + 2, 0, clipmapResolution * 4));
		}

		for (int i = clipmapResolution * 4; i > 0; i -= 2) {

			outerDegenerateVerts.push_back(glm::vec3(clipmapResolution * 4, 0, i));
			outerDegenerateVerts.push_back(glm::vec3(clipmapResolution * 4, 0, i - 1));
			outerDegenerateVerts.push_back(glm::vec3(clipmapResolution * 4, 0, i - 2));
		}

		for (int i = clipmapResolution * 4; i > 0; i -= 2) {

			outerDegenerateVerts.push_back(glm::vec3(i, 0, 0));
			outerDegenerateVerts.push_back(glm::vec3(i - 1, 0, 0));
			outerDegenerateVerts.push_back(glm::vec3(i - 2, 0, 0));
		}

		for (int i = 0; i < clipmapResolution * 3 * 2 * 4; i++)
			outerDegenerateIndices.push_back(i);

		for (int i = clipmapResolution * 3 * 2 * 4 - 1; i >= 0; i--)
			outerDegenerateIndices.push_back(i);

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
		glew->genVertexArrays(1, &ringFixUpHorizontalVAO);
		glew->bindVertexArray(ringFixUpHorizontalVAO);

		unsigned int ringFixUpVBO;
		glew->genBuffers(1, &ringFixUpVBO);
		glew->bindBuffer(0x8892, ringFixUpVBO);
		glew->bufferData(0x8892, ringFixUpVerts.size() * sizeof(glm::vec3), &ringFixUpVerts[0], 0x88E4);
		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int ringFixUpEBO;
		glew->genBuffers(1, &ringFixUpEBO);
		glew->bindBuffer(0x8893, ringFixUpEBO);
		glew->bufferData(0x8893, ringFixUpHorizontalIndices.size() * sizeof(unsigned int), &ringFixUpHorizontalIndices[0], 0x88E4);

		// Ring Fix-up (Vertical)
		glew->genVertexArrays(1, &ringFixUpVerticalVAO);
		glew->bindVertexArray(ringFixUpVerticalVAO);

		unsigned int ringFixUpVBO1;
		glew->genBuffers(1, &ringFixUpVBO1);
		glew->bindBuffer(0x8892, ringFixUpVBO1);
		glew->bufferData(0x8892, ringFixUpVerts1.size() * sizeof(glm::vec3), &ringFixUpVerts1[0], 0x88E4);
		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int ringFixUpEBO1;
		glew->genBuffers(1, &ringFixUpEBO1);
		glew->bindBuffer(0x8893, ringFixUpEBO1);
		glew->bufferData(0x8893, ringFixUpVerticalIndices.size() * sizeof(unsigned int), &ringFixUpVerticalIndices[0], 0x88E4);

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
	}

	void Terrain::setBoundariesOfClipmap(const int& level, glm::vec3& start, glm::vec3& end) {

		glm::vec2 bounds;
		Terrain::getMaxAndMinHeights(bounds, level, start, end);
		start.y = bounds.x;
		end.y = bounds.y;
	}

	void Terrain::getMaxAndMinHeights(glm::vec2& bounds, const int& level, const glm::vec3& start, const glm::vec3& end) {

		bounds.x = 999999;
		bounds.y = -999999;

		int startX = (int)start.x >> level;
		int endX = (int)end.x >> level;
		int startZ = (int)start.z >> level;
		int endZ = (int)end.z >> level;
		int mipMapsize = elevationMapSize >> level;
		for (int i = startZ; i < endZ; i++) {

			for (int j = startX; j < endX; j++) {

				float height = heightMipMaps[level][i * mipMapsize + j];
				if (height < bounds.x) bounds.x = height;
				if (height > bounds.y) bounds.y = height;
			}
		}
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

	float** Terrain::getFlatHeightmap() {

		int mipmaps = 0;
		for (int i = elevationMapSize; i > 1; i >>= 1)
			mipmaps++;

		float** heightMipMaps = new float* [mipmaps];

		mipmaps = 0;
		for (int size = elevationMapSize; size > 1; size >>= 1) {

			float* data = new float[size * size];

			for (int i = 0; i < size; i++)
				for (int j = 0; j < size; j++)
					data[i * size + j] = 0;

			heightMipMaps[mipmaps] = data;
			mipmaps++;
		}

		return heightMipMaps;
	}

}