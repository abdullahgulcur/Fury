#include "pch.h"
#include "renderer.h"
#include "editor.h"
#include "scenecamera.h"
#include "component/gamecamera.h"
#include "entity.h"
#include "scene.h"
#include "glm/glm.hpp"

#define TERRAIN_INSTANCED_RENDERING

#define RESOLUTION 16384
#define TILE_SIZE 256
#define MEM_TILE_ONE_SIDE 6

using namespace std::chrono;


#define ASSERT(x) if(!(x)) __debugbreak;
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))

static void GLClearError() {

	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line) {

	while (GLenum error = glGetError()) {

		std::cout << "[OpenGL Error] (" << error << "): " << function << " " << file << ":" << line << std::endl;
		return false;
	}
	return true;
}


namespace Editor {

	Renderer::Renderer() {

	}

	Renderer::~Renderer() {

		std::cout << "Average " << (float)total / counter << std::endl;
	}

	void Renderer::init() {

		Renderer::initDefaultSphere();
		pickingProgramID = Core::instance->glewContext->loadShaders("C:/Projects/Fury/Editor/src/shader/ObjectPick.vert",
			"C:/Projects/Fury/Editor/src/shader/ObjectPick.frag");
	}

	void Renderer::initDefaultSphere() {

        GlewContext* glewContext = Core::instance->glewContext;

        unsigned int vbo, ebo;
        glewContext->genVertexArrays(1, &defaultSphereVAO);
        glewContext->genBuffers(1, &vbo);
        glewContext->genBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        defaultSphereIndexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glewContext->bindVertexArray(defaultSphereVAO);
        glewContext->bindArrayBuffer(vbo);
        glewContext->arrayBufferData(data);
        glewContext->bindElementArrayBuffer(ebo);
        glewContext->elementArrayBufferData(indices);
        glewContext->enableVertexAttribArray(0);
        glewContext->vertexAttribPointer(0, 3, stride, (void*)0);
        glewContext->enableVertexAttribArray(1);
        glewContext->vertexAttribPointer(1, 3, stride, (void*)(3 * sizeof(float)));
        glewContext->enableVertexAttribArray(2);
        glewContext->vertexAttribPointer(2, 2, stride, (void*)(6 * sizeof(float)));
	}

	void Renderer::update() {

		/* reset */
		drawCallCount = 0;

		Scene* scene = Core::instance->sceneManager->currentScene;
		if (!scene)
			return;

		SceneCamera* camera = Editor::instance->sceneCamera;
		if (!camera)
			return;

		GlewContext* glew = Core::instance->glewContext;
		PBRMaterial* pbrMaterial = Core::instance->fileSystem->pbrMaterial;

		glm::mat4& VP = camera->projectionViewMatrix;
		glm::mat4& view = camera->ViewMatrix;
		glm::mat4& projection = camera->ProjectionMatrix;
		glm::vec3& camPos = camera->position;

		//-----

		unsigned int pbrShaderProgramId = pbrMaterial->pbrShaderProgramId;
		glew->useProgram(pbrShaderProgramId);
		glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "projection"), 1, GL_FALSE, &projection[0][0]);

		unsigned int backgroundShaderProgramId = pbrMaterial->backgroundShaderProgramId;
		glew->useProgram(backgroundShaderProgramId);
		glew->uniformMatrix4fv(glew->getUniformLocation(backgroundShaderProgramId, "projection"), 1, GL_FALSE, &projection[0][0]);

		//glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, "camPos"), 1, &camPos[0]);
		//glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "projection"), 1, GL_FALSE, &projection[0][0]);
		//glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "view"), 1, GL_FALSE, &view[0][0]);

		//// render light source (simply re-render sphere at light positions)
		//// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
		//// keeps the codeprint small.
		//for (unsigned int i = 0; i < sizeof(pbrMaterial->lightPositions) / sizeof(pbrMaterial->lightPositions[0]); ++i) {

		//	glm::vec3 newPos = pbrMaterial->lightPositions[i];
		//	std::string str = "lightPositions[" + std::to_string(i) + "]";
		//	glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, &str[0]), 1, &newPos[0]);
		//	str = "lightColors[" + std::to_string(i) + "]";
		//	glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, &str[0]), 1, &pbrMaterial->lightColors[i][0]);
		//}

		//// bind pre-computed IBL data
		//glew->activeTexture(GL_TEXTURE0);
		//glew->bindTexture(GL_TEXTURE_CUBE_MAP, pbrMaterial->irradianceMap);
		//glew->activeTexture(GL_TEXTURE1);
		//glew->bindTexture(GL_TEXTURE_CUBE_MAP, pbrMaterial->prefilterMap);
		//glew->activeTexture(GL_TEXTURE2);
		//glew->bindTexture(GL_TEXTURE_2D, pbrMaterial->brdfLUTTexture);

		//-----
		Core::instance->glewContext->bindFrameBuffer(camera->FBO);
		Core::instance->glewContext->enable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
		Core::instance->glewContext->depthFunc(GL_LEQUAL); // set depth function to less than AND equal for skybox depth trick.

		Core::instance->glewContext->enable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // enable depth testing (is disabled for rendering screen-space quad)
		//glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
		Core::instance->glewContext->viewport(Editor::instance->menu->sceneRegion.x, Editor::instance->menu->sceneRegion.y);
		Core::instance->glewContext->clearScreen(glm::vec3(0.3f, 0.3f, 0.3f));

		glew->useProgram(pbrShaderProgramId);
		glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, "camPos"), 1, &camPos[0]);
		glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "view"), 1, GL_FALSE, &view[0][0]);

		// bind pre-computed IBL data
		glew->activeTexture(GL_TEXTURE0);
		glew->bindTexture(GL_TEXTURE_CUBE_MAP, pbrMaterial->irradianceMap);
		glew->activeTexture(GL_TEXTURE1);
		glew->bindTexture(GL_TEXTURE_CUBE_MAP, pbrMaterial->prefilterMap);
		glew->activeTexture(GL_TEXTURE2);
		glew->bindTexture(GL_TEXTURE_2D, pbrMaterial->brdfLUTTexture);

		std::stack<Entity*> entStack;
		entStack.push(scene->root);

		while (!entStack.empty()) {

			Entity* popped = entStack.top();
			entStack.pop();

			/* Gizmo Part */
			//GameCamera* gamecamera = popped->getComponent<GameCamera>();
			//if (gamecamera != NULL && Editor::instance->menu->selectedEntity == popped) // bunu ayir
			//	gamecamera->drawEditorGizmos(VP, popped->transform->model);
			/* Gizmo Part End*/

			//Terrain* terrain = popped->getComponent<Terrain>();
			//if (terrain != NULL && scene->primaryCamera != NULL) {
			//	terrain->update(); // that asshole should not stay here...
			//	//glew->polygonMode(GL_FRONT_AND_BACK, GL_LINE);


			//	auto start = high_resolution_clock::now();

			//	Renderer::drawTerrain(Editor::instance->sceneCamera, scene->primaryCamera, terrain);

			//	auto stop = high_resolution_clock::now();
			//	auto duration = duration_cast<microseconds>(stop - start);

			//	total += duration.count();
			//	counter++;
			//	//printf("Time taken by terrain draw function (instanced): %d\n", duration.count());
			//	//std::cout << "Time taken by terrain draw function (instanced): " << duration.count() << " microseconds" << std::endl;

			//	//glew->polygonMode(GL_BACK, GL_TRIANGLES);
			//}

			

			for (Transform*& child : popped->transform->children)
				entStack.push(child->entity);

			MeshRenderer* renderer = popped->getComponent<MeshRenderer>();
			if (!renderer)
				continue;

			MeshFile* mesh = renderer->meshFile;
			MaterialFile* mat = renderer->materialFile;
			if (!mesh || !mat)
				continue;

			glm::mat4 model = popped->transform->model;
			glm::vec4 startInWorldSpace = model * mesh->aabbBox.start;
			glm::vec4 endInWorldSpace = model * mesh->aabbBox.end;

			if (!camera->intersectsAABB(startInWorldSpace, endInWorldSpace))
				continue;

			switch (mat->shaderType) {

			case ShaderType::PBR: {

				glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "model"), 1, GL_FALSE, &model[0][0]);

				for (int i = 0; i < mat->textureFiles.size(); i++) {

					std::string texStr = "texture" + std::to_string(i);
					glew->activeTexture(GL_TEXTURE0 + i + 3);
					glew->bindTexture(GL_TEXTURE_2D, mat->textureFiles[i]->textureId);
				}

				glew->bindVertexArray(mesh->VAO);
				glew->drawElements(GL_TRIANGLES, mesh->indiceCount, GL_UNSIGNED_INT, (void*)0);
				//glew->bindVertexArray(0);

				break;
			}
			case ShaderType::PBR_ALPHA: {
				break;
			}
			}

			drawCallCount++;
		}

		// render light source (simply re-render sphere at light positions)
		// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
		// keeps the codeprint small.
		for (unsigned int i = 0; i < sizeof(pbrMaterial->lightPositions) / sizeof(pbrMaterial->lightPositions[0]); ++i) {

			glm::vec3 newPos = pbrMaterial->lightPositions[i];
			std::string str = "lightPositions[" + std::to_string(i) + "]";
			glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, &str[0]), 1, &newPos[0]);
			str = "lightColors[" + std::to_string(i) + "]";
			glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, &str[0]), 1, &pbrMaterial->lightColors[i][0]);
		}

		// render skybox (render as last to prevent overdraw)
		glew->useProgram(backgroundShaderProgramId);
		glew->uniformMatrix4fv(glew->getUniformLocation(backgroundShaderProgramId, "view"), 1, GL_FALSE, &view[0][0]);
		glew->activeTexture(GL_TEXTURE0);
		glew->bindTexture(GL_TEXTURE_CUBE_MAP, pbrMaterial->envCubemap);
		//glew->uniform1i(glew->getUniformLocation(backgroundShaderProgramId, "environmentMap"), 0);

		//glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap); // display irradiance map
		//glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap); // display prefilter map
		pbrMaterial->renderCube();

		Core::instance->glewContext->bindFrameBuffer(0);
	}

	//void Renderer::update() {

	//	/* reset */
	//	drawCallCount = 0;

	//	Scene* scene = Core::instance->sceneManager->currentScene;
	//	if (!scene)
	//		return;

	//	SceneCamera* camera = Editor::instance->sceneCamera;
	//	if (!camera)
	//		return;

	//	GlewContext* glew = Core::instance->glewContext;
	//	PBRMaterial* pbrMaterial = Core::instance->fileSystem->pbrMaterial;

	//	glm::mat4& VP = camera->projectionViewMatrix;
	//	glm::mat4& view = camera->ViewMatrix;
	//	glm::mat4& projection = camera->ProjectionMatrix;
	//	glm::vec3& camPos = camera->position;

	//	//-----

	//	unsigned int pbrShaderProgramId = pbrMaterial->pbrShaderProgramId;
	//	glew->useProgram(pbrShaderProgramId);
	//	glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "projection"), 1, GL_FALSE, &projection[0][0]);

	//	unsigned int backgroundShaderProgramId = pbrMaterial->backgroundShaderProgramId;
	//	glew->useProgram(backgroundShaderProgramId);
	//	glew->uniformMatrix4fv(glew->getUniformLocation(backgroundShaderProgramId, "projection"), 1, GL_FALSE, &projection[0][0]);

	//	//glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, "camPos"), 1, &camPos[0]);
	//	//glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "projection"), 1, GL_FALSE, &projection[0][0]);
	//	//glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "view"), 1, GL_FALSE, &view[0][0]);

	//	//// render light source (simply re-render sphere at light positions)
	//	//// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
	//	//// keeps the codeprint small.
	//	//for (unsigned int i = 0; i < sizeof(pbrMaterial->lightPositions) / sizeof(pbrMaterial->lightPositions[0]); ++i) {

	//	//	glm::vec3 newPos = pbrMaterial->lightPositions[i];
	//	//	std::string str = "lightPositions[" + std::to_string(i) + "]";
	//	//	glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, &str[0]), 1, &newPos[0]);
	//	//	str = "lightColors[" + std::to_string(i) + "]";
	//	//	glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, &str[0]), 1, &pbrMaterial->lightColors[i][0]);
	//	//}

	//	//// bind pre-computed IBL data
	//	//glew->activeTexture(GL_TEXTURE0);
	//	//glew->bindTexture(GL_TEXTURE_CUBE_MAP, pbrMaterial->irradianceMap);
	//	//glew->activeTexture(GL_TEXTURE1);
	//	//glew->bindTexture(GL_TEXTURE_CUBE_MAP, pbrMaterial->prefilterMap);
	//	//glew->activeTexture(GL_TEXTURE2);
	//	//glew->bindTexture(GL_TEXTURE_2D, pbrMaterial->brdfLUTTexture);

	//	//-----
	//	Core::instance->glewContext->bindFrameBuffer(camera->FBO);
	//	Core::instance->glewContext->enable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
	//	Core::instance->glewContext->depthFunc(GL_LEQUAL); // set depth function to less than AND equal for skybox depth trick.

	//	Core::instance->glewContext->enable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // enable depth testing (is disabled for rendering screen-space quad)
	//	//glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
	//	Core::instance->glewContext->viewport(Editor::instance->menu->sceneRegion.x, Editor::instance->menu->sceneRegion.y);
	//	Core::instance->glewContext->clearScreen(glm::vec3(0.3f, 0.3f, 0.3f));

	//	glew->useProgram(pbrShaderProgramId);
	//	glew->uniform3fv(glew->getUniformLocation(pbrShaderProgramId, "camPos"), 1, &camPos[0]);
	//	glew->uniformMatrix4fv(glew->getUniformLocation(pbrShaderProgramId, "view"), 1, GL_FALSE, &view[0][0]);


	//	// render skybox (render as last to prevent overdraw)
	//	glew->useProgram(backgroundShaderProgramId);
	//	glew->uniformMatrix4fv(glew->getUniformLocation(backgroundShaderProgramId, "view"), 1, GL_FALSE, &view[0][0]);
	//	glew->activeTexture(GL_TEXTURE0);
	//	glew->bindTexture(GL_TEXTURE_CUBE_MAP, pbrMaterial->envCubemap);
	//	pbrMaterial->renderCube();

	//	Core::instance->glewContext->bindFrameBuffer(0);
	//}

	//void Renderer::drawMeshRendererRecursively(Entity* entity, glm::mat4& PV, glm::vec3& camPos) {

	//	MeshRenderer* renderer = entity->getComponent<MeshRenderer>();
	//	Terrain* terrain = entity->getComponent<Terrain>();
	//	GameCamera* gamecamera = entity->getComponent<GameCamera>();
	//	glm::mat4 model = entity->transform->model;
	//	glm::mat4& VP = PV;
	//	GlewContext* glew = Core::instance->glewContext;

	//	if (renderer != NULL) {
	//		if (renderer->meshFile && renderer->materialFile) {

	//			unsigned int programId = renderer->materialFile->programId;
	//			glew->useProgram(programId);
	//			glew->uniform3fv(glew->getUniformLocation(programId, "camPos"), 1, &camPos[0]);
	//			glew->uniformMatrix4fv(glew->getUniformLocation(programId, "PV"), 1, 0, &VP[0][0]);
	//			glew->uniformMatrix4fv(glew->getUniformLocation(programId, "model"), 1, 0, &model[0][0]);

	//			if (renderer->materialFile->shaderType == ShaderType::PBR) {

	//				for (int i = 0; i < renderer->materialFile->activeTextureIndices.size(); i++) {

	//					std::string texStr = "texture" + std::to_string(renderer->materialFile->activeTextureIndices[i]);
	//					glew->activeTexture(0x84C0 + i);
	//					glew->bindTexture(0x0DE1, renderer->materialFile->textureFiles[i]->textureId);
	//					glew->uniform1i(glew->getUniformLocation(programId, &texStr[0]), i);
	//				}	
	//			}

	//			glew->bindVertexArray(renderer->meshFile->VAO);
	//			glew->drawElements(0x0004, renderer->meshFile->indiceCount, 0x1405, (void*)0);
	//			glew->bindVertexArray(0);
	//		}
	//	}

	//	glew->polygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//	if (terrain != NULL && gamecamera != NULL)
	//		Renderer::drawTerrain(Editor::instance->sceneCamera, gamecamera, terrain);
	//	glew->polygonMode(GL_BACK, GL_FILL);

	//	if (gamecamera != NULL && Editor::instance->menu->selectedEntity == entity) // bunu ayir
	//		gamecamera->drawEditorGizmos(PV, entity->transform->model);

	//	for (auto& transform : entity->transform->children)
	//		Renderer::drawMeshRendererRecursively(transform->entity, PV, camPos);
	//}




	void Renderer::drawTerrain(SceneCamera* camera, GameCamera* gc, Terrain* terrain) { // GameCamera* gc, , Terrain* terrain

		GlewContext* glew = Core::instance->glewContext;

		int count = 0;
		//float scale = terrain->triangleSize;
		int level = terrain->getMaxMipLevel(RESOLUTION, TILE_SIZE);// terrain->clipmapLevel;
		int clipmapResolution = terrain->clipmapResolution;
		//int worldSize = scale * clipmapResolution * (2 << level);
		//unsigned int elevationMapSize = terrain->elevationMapSize;
		int programID = terrain->programID;
		float* pvAddr = &camera->projectionViewMatrix[0][0];

		// Change this for debugging purposes--------
		//glm::vec3 camPos = camera->position;
		float fake = 1000000;
		glm::vec3 fakeDisplacement = glm::vec3(fake, 0, fake);
		glm::vec3 camPos = gc->position + fakeDisplacement;
		//-------------------------------------------

		float* lightDirAddr = &glm::normalize(-glm::vec3(0.5, -1, 0.5))[0];
		float* lightColAddr = &glm::vec3(1, 1, 1)[0];

		int elevationMapTexture = terrain->elevationMapTexture;
		int diffuseMapTexture = terrain->diffuseMapTexture;
		//int normalMapTexture = terrain->normalMapTexture;

		glew->useProgram(programID);
		glew->uniformMatrix4fv(glew->getUniformLocation(programID, "PV"), 1, 0, pvAddr);

		//glew->uniform1i(glew->getUniformLocation(programID, "patchRes"), level);

		//glew->uniform1i(glew->getUniformLocation(programID, "mapSize"), elevationMapSize);
		//glew->uniform1i(glew->getUniformLocation(programID, "clipMapSize"), worldSize);
		glew->uniform3fv(glew->getUniformLocation(programID, "camPos"), 1, &camera->position[0]);
		glew->uniform3fv(glew->getUniformLocation(programID, "lightDir"), 1, lightDirAddr);
		//glew->uniform3fv(glew->getUniformLocation(programID, "lightColor"), 1, lightColAddr);
		//glew->uniform1f(glew->getUniformLocation(programID, "triSize"), scale);

		glew->activeTexture(0x84C0);
		glew->bindTexture(0x0DE1, diffuseMapTexture);
		glew->uniform1i(glew->getUniformLocation(programID, "grassTex"), 0);

		glew->activeTexture(0x84C1);
		//glew->bindTexture(0x0DE1, elevationMapTexture);
		glew->bindTexture(0x8C1A, elevationMapTexture);
		//glew->uniform1i(glew->getUniformLocation(programID, "heightmap"), 0);
		glew->uniform1i(glew->getUniformLocation(programID, "heightmapArray"), 1);


		//glew->activeTexture(0x84C1);
		//glew->bindTexture(0x0DE1, normalMapTexture);
		//glew->uniform1i(glew->getUniformLocation(programID, "normalmap"), 1);

		glew->useProgram(programID);

		int blockVAO = terrain->blockVAO;
		int ringFixUpVAO = terrain->ringFixUpVAO;
		int smallSquareVAO = terrain->smallSquareVAO;
		int interiorTrimVAO = terrain->interiorTrimVAO;
		int outerDegenerateVAO = terrain->outerDegenerateVAO;

		int blockIndiceCount = terrain->blockIndices.size();
		int ringFixUpIndiceCount = terrain->ringFixUpIndices.size();
		int smallSquareIndiceCount = terrain->smallSquareIndices.size();
		int interiorTrimIndiceCount = terrain->interiorTrimIndices.size();
		int outerDegenerateIndiceCount = terrain->outerDegenerateIndices.size();
		
		//Renderer::drawInternalPart(programID, blockVAO, ringFixUpVAO, smallSquareVAO, blockIndiceCount, 
		//	ringFixUpIndiceCount, smallSquareIndiceCount, clipmapResolution, camPos, fakeDisplacement);

		// It has to be two. 
		int patchWidth = 2;

		// '4' has to be constant, because every level has 4 block at each side.
		//int wholeClipmapRegionSize = clipmapResolution * 4 * (1 << level);

		glm::vec2* blockPositions = terrain->blockPositions;
		glm::vec2* ringFixUpPositions = terrain->ringFixUpPositions;
		glm::vec2* interiorTrimPositions = terrain->interiorTrimPositions;
		glm::vec2* outerDegeneratePositions = terrain->outerDegeneratePositions;
		float* rotAmounts = terrain->rotAmounts;
		glm::vec2 smallSquarePosition = terrain->smallSquarePosition;

		/// <summary>
		/// NE KADAR TEKRARLAMA VARSA, HEPSINI KALDIR...
		/// </summary>
		/// <param name="camera"></param>
		/// <param name="gc"></param>
		/// <param name="terrain"></param>

#ifdef TERRAIN_INSTANCED_RENDERING

		std::vector<TerrainVertexAttribs> instanceArray;

		// BLOCKS

		for (int i = 0; i < level; i++) {

			for (int j = 0; j < 12; j++) {

				glm::vec4 startInWorldSpace;
				glm::vec4 endInWorldSpace;
				AABB_Box aabb = terrain->blockAABBs[i * 12 + j];
				startInWorldSpace = aabb.start;
				endInWorldSpace = aabb.end;
				if (gc->intersectsAABB(startInWorldSpace, endInWorldSpace)) {

					TerrainVertexAttribs attribs;
					attribs.level = i;
					attribs.model = glm::mat4(1);
					attribs.position = glm::vec2(blockPositions[i * 12 + j].x, blockPositions[i * 12 + j].y);
					attribs.scale = (1 << i);
					attribs.texSize = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2) * (1 << i);
					attribs.texturePos = glm::vec2((float)terrain->initialTexturePositions[i].x, (float)terrain->initialTexturePositions[i].z);
					instanceArray.push_back(attribs);
				}
			}
		}


		for (int i = 0; i < 4; i++) {

			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			attribs.position = glm::vec2(blockPositions[level * 12 + i].x, blockPositions[level * 12 + i].y);
			attribs.scale = 1;
			attribs.texSize = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2);
			attribs.texturePos = glm::vec2((float)terrain->initialTexturePositions[0].x, (float)terrain->initialTexturePositions[0].z);
			instanceArray.push_back(attribs);
		}

		unsigned int instanceBuffer;
		glew->genBuffers(1, &instanceBuffer);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->bufferData(0x8892, instanceArray.size() * sizeof(TerrainVertexAttribs), &instanceArray[0], 0x88E4);

		int size = sizeof(TerrainVertexAttribs);

		glew->bindVertexArray(blockVAO);
		glew->enableVertexAttribArray(1);
		glew->vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, size, (void*)0);
		glew->enableVertexAttribArray(2);
		glew->vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2)));
		glew->enableVertexAttribArray(3);
		glew->vertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2));
		glew->enableVertexAttribArray(4);
		glew->vertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float)));
		glew->enableVertexAttribArray(5);
		glew->vertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 2));
		glew->enableVertexAttribArray(6);
		glew->vertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3));
		glew->enableVertexAttribArray(7);																			
		glew->vertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4)));
		glew->enableVertexAttribArray(8);																			
		glew->vertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 2));
		glew->enableVertexAttribArray(9);																			
		glew->vertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 3));
		glew->vertexAttribDivisor(1, 1);
		glew->vertexAttribDivisor(2, 1);
		glew->vertexAttribDivisor(3, 1);
		glew->vertexAttribDivisor(4, 1);
		glew->vertexAttribDivisor(5, 1);
		glew->vertexAttribDivisor(6, 1);
		glew->vertexAttribDivisor(7, 1);
		glew->vertexAttribDivisor(8, 1);
		glew->vertexAttribDivisor(9, 1);

		glew->drawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(blockIndiceCount), GL_UNSIGNED_INT, 0, instanceArray.size());

		glew->bindVertexArray(0);
		glew->deleteBuffers(1, &instanceBuffer);
		instanceArray.clear();

		// RING FIXUP

		for (int i = 0; i < level; i++) {

			glm::mat4 model = glm::mat4(1);

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = model;
			
			attribs.scale = (1 << i);
			attribs.texSize = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2) * (1 << i);
			attribs.texturePos = glm::vec2((float)terrain->initialTexturePositions[i].x, (float)terrain->initialTexturePositions[i].z);

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
			attribs.scale = 1;
			attribs.texSize = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2);
			attribs.texturePos = glm::vec2((float)terrain->initialTexturePositions[0].x, (float)terrain->initialTexturePositions[0].z);

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
		
		glew->genBuffers(1, &instanceBuffer);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->bufferData(0x8892, instanceArray.size() * sizeof(TerrainVertexAttribs), &instanceArray[0], 0x88E4);

		glew->bindVertexArray(ringFixUpVAO);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->enableVertexAttribArray(1);
		glew->vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, size, (void*)0);
		glew->enableVertexAttribArray(2);
		glew->vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2)));
		glew->enableVertexAttribArray(3);
		glew->vertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2));
		glew->enableVertexAttribArray(4);
		glew->vertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float)));
		glew->enableVertexAttribArray(5);
		glew->vertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 2));
		glew->enableVertexAttribArray(6);
		glew->vertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3));
		glew->enableVertexAttribArray(7);
		glew->vertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4)));
		glew->enableVertexAttribArray(8);
		glew->vertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 2));
		glew->enableVertexAttribArray(9);
		glew->vertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 3));
		glew->vertexAttribDivisor(1, 1);
		glew->vertexAttribDivisor(2, 1);
		glew->vertexAttribDivisor(3, 1);
		glew->vertexAttribDivisor(4, 1);
		glew->vertexAttribDivisor(5, 1);
		glew->vertexAttribDivisor(6, 1);
		glew->vertexAttribDivisor(7, 1);
		glew->vertexAttribDivisor(8, 1);
		glew->vertexAttribDivisor(9, 1);

		glew->drawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(ringFixUpIndiceCount), GL_UNSIGNED_INT, 0, instanceArray.size());

		glew->bindVertexArray(0);
		glew->deleteBuffers(1, &instanceBuffer);
		instanceArray.clear();

		// INTERIOR TRIM

		for (int i = 0; i < level; i++) {

			glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(rotAmounts[i]), glm::vec3(0.0f, 1.0f, 0.0f));

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = model;
			attribs.scale = (2 << i);
			attribs.texSize = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2) * (1 << i);
			attribs.texturePos = glm::vec2((float)terrain->initialTexturePositions[i].x, (float)terrain->initialTexturePositions[i].z);
			attribs.position = glm::vec2(interiorTrimPositions[i].x, interiorTrimPositions[i].y);
			instanceArray.push_back(attribs);
		}

		glew->genBuffers(1, &instanceBuffer);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->bufferData(0x8892, instanceArray.size() * sizeof(TerrainVertexAttribs), &instanceArray[0], 0x88E4);

		glew->bindVertexArray(interiorTrimVAO);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->enableVertexAttribArray(1);
		glew->vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, size, (void*)0);
		glew->enableVertexAttribArray(2);
		glew->vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2)));
		glew->enableVertexAttribArray(3);
		glew->vertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2));
		glew->enableVertexAttribArray(4);
		glew->vertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float)));
		glew->enableVertexAttribArray(5);
		glew->vertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 2));
		glew->enableVertexAttribArray(6);
		glew->vertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3));
		glew->enableVertexAttribArray(7);
		glew->vertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4)));
		glew->enableVertexAttribArray(8);
		glew->vertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 2));
		glew->enableVertexAttribArray(9);
		glew->vertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 3));
		glew->vertexAttribDivisor(1, 1);
		glew->vertexAttribDivisor(2, 1);
		glew->vertexAttribDivisor(3, 1);
		glew->vertexAttribDivisor(4, 1);
		glew->vertexAttribDivisor(5, 1);
		glew->vertexAttribDivisor(6, 1);
		glew->vertexAttribDivisor(7, 1);
		glew->vertexAttribDivisor(8, 1);
		glew->vertexAttribDivisor(9, 1);

		glew->drawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(interiorTrimIndiceCount), GL_UNSIGNED_INT, 0, instanceArray.size());

		glew->bindVertexArray(0);
		glew->deleteBuffers(1, &instanceBuffer);
		instanceArray.clear();

		// OUTER DEGENERATE

		for (int i = 0; i < level; i++) {

			glm::mat4 model = glm::mat4(1);

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = model;
			attribs.scale = (1 << i);
			attribs.texSize = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2) * (1 << i);
			attribs.texturePos = glm::vec2((float)terrain->initialTexturePositions[i].x, (float)terrain->initialTexturePositions[i].z);

			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 0].x, outerDegeneratePositions[i * 4 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 1].x, outerDegeneratePositions[i * 4 + 1].y);
			instanceArray.push_back(attribs);

			model = glm::rotate(glm::mat4(1), glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f));

			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 2].x, outerDegeneratePositions[i * 4 + 2].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 3].x, outerDegeneratePositions[i * 4 + 3].y);
			instanceArray.push_back(attribs);
		}

		glew->genBuffers(1, &instanceBuffer);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->bufferData(0x8892, instanceArray.size() * sizeof(TerrainVertexAttribs), &instanceArray[0], 0x88E4);

		glew->bindVertexArray(outerDegenerateVAO);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->enableVertexAttribArray(1);
		glew->vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, size, (void*)0);
		glew->enableVertexAttribArray(2);
		glew->vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2)));
		glew->enableVertexAttribArray(3);
		glew->vertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2));
		glew->enableVertexAttribArray(4);
		glew->vertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float)));
		glew->enableVertexAttribArray(5);
		glew->vertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 2));
		glew->enableVertexAttribArray(6);
		glew->vertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3));
		glew->enableVertexAttribArray(7);
		glew->vertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4)));
		glew->enableVertexAttribArray(8);
		glew->vertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 2));
		glew->enableVertexAttribArray(9);
		glew->vertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 3));
		glew->vertexAttribDivisor(1, 1);
		glew->vertexAttribDivisor(2, 1);
		glew->vertexAttribDivisor(3, 1);
		glew->vertexAttribDivisor(4, 1);
		glew->vertexAttribDivisor(5, 1);
		glew->vertexAttribDivisor(6, 1);
		glew->vertexAttribDivisor(7, 1);
		glew->vertexAttribDivisor(8, 1);
		glew->vertexAttribDivisor(9, 1);

		glew->drawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(outerDegenerateIndiceCount), GL_UNSIGNED_INT, 0, instanceArray.size());

		glew->bindVertexArray(0);
		glew->deleteBuffers(1, &instanceBuffer);
		instanceArray.clear();

		// SMALL SQUARE

		TerrainVertexAttribs attribs;
		attribs.level = 0;
		attribs.model = glm::mat4(1);
		attribs.scale = 1;
		attribs.texSize = TILE_SIZE * (MEM_TILE_ONE_SIDE - 2);
		attribs.texturePos = glm::vec2((float)terrain->initialTexturePositions[0].x, (float)terrain->initialTexturePositions[0].z);

		attribs.position = glm::vec2(smallSquarePosition.x, smallSquarePosition.y);
		instanceArray.push_back(attribs);

		glew->genBuffers(1, &instanceBuffer);
		glew->bindBuffer(0x8892, instanceBuffer);
		glew->bufferData(0x8892, instanceArray.size() * sizeof(TerrainVertexAttribs), &instanceArray[0], 0x88E4);

		glew->bindVertexArray(smallSquareVAO);
		glew->enableVertexAttribArray(1);
		glew->vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, size, (void*)0);
		glew->enableVertexAttribArray(2);
		glew->vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2)));
		glew->enableVertexAttribArray(3);
		glew->vertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2));
		glew->enableVertexAttribArray(4);
		glew->vertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float)));
		glew->enableVertexAttribArray(5);
		glew->vertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 2));
		glew->enableVertexAttribArray(6);
		glew->vertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3));
		glew->enableVertexAttribArray(7);
		glew->vertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4)));
		glew->enableVertexAttribArray(8);
		glew->vertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 2));
		glew->enableVertexAttribArray(9);
		glew->vertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, size, (void*)((sizeof(glm::vec2) * 2) + sizeof(float) * 3 + sizeof(glm::vec4) * 3));
		glew->vertexAttribDivisor(1, 1);
		glew->vertexAttribDivisor(2, 1);
		glew->vertexAttribDivisor(3, 1);
		glew->vertexAttribDivisor(4, 1);
		glew->vertexAttribDivisor(5, 1);
		glew->vertexAttribDivisor(6, 1);
		glew->vertexAttribDivisor(7, 1);
		glew->vertexAttribDivisor(8, 1);
		glew->vertexAttribDivisor(9, 1);

		glew->drawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(smallSquareIndiceCount), GL_UNSIGNED_INT, 0, instanceArray.size());

		glew->bindVertexArray(0);
		glew->deleteBuffers(1, &instanceBuffer);
		instanceArray.clear();

#endif

#ifndef TERRAIN_INSTANCED_RENDERING 


		for (int i = 0; i < level; i++) {

			glew->uniform1f(glew->getUniformLocation(programID, "texSize"), TILE_SIZE * (MEM_TILE_ONE_SIDE - 2) * (1 << i));
			glew->uniform1i(glew->getUniformLocation(programID, "level"), i);
			glew->uniform2f(glew->getUniformLocation(programID, "texturePos"), (float)terrain->initialTexturePositions[i].x, (float)terrain->initialTexturePositions[i].z);
			//glew->uniform2f(glew->getUniformLocation(programID, "texturePos"), (float)terrain->currentTileIndices[i].x * TILE_SIZE * (1 << i), (float)terrain->currentTileIndices[i].z * TILE_SIZE * (1 << i));

			// BLOCKS
			glew->uniform1f(glew->getUniformLocation(programID, "scale"), 1 << i);
			glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 0, 1)[0]);
			glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &glm::mat4(1)[0][0]);
			glew->bindVertexArray(blockVAO);

			for (int j = 0; j < 12; j++) {

				glm::vec4 startInWorldSpace;
				glm::vec4 endInWorldSpace;
				AABB_Box aabb = terrain->blockAABBs[i * 12 + j];
				startInWorldSpace = aabb.start;
				endInWorldSpace = aabb.end;
				if (gc->intersectsAABB(startInWorldSpace, endInWorldSpace))
					Renderer::drawPartOfTerrain(glew, blockPositions[i * 12 + j].x, blockPositions[i * 12 + j].y, programID, blockIndiceCount);
			}

			// RING FIXUP
			glew->bindVertexArray(ringFixUpVAO);
			//glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 1, 1)[0]);

			Renderer::drawPartOfTerrain(glew, ringFixUpPositions[i * 4 + 0].x, ringFixUpPositions[i * 4 + 0].y, programID, ringFixUpIndiceCount);
			Renderer::drawPartOfTerrain(glew, ringFixUpPositions[i * 4 + 2].x, ringFixUpPositions[i * 4 + 2].y, programID, ringFixUpIndiceCount);
			glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
			glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &model[0][0]);
			Renderer::drawPartOfTerrain(glew, ringFixUpPositions[i * 4 + 1].x, ringFixUpPositions[i * 4 + 1].y, programID, ringFixUpIndiceCount);
			Renderer::drawPartOfTerrain(glew, ringFixUpPositions[i * 4 + 3].x, ringFixUpPositions[i * 4 + 3].y, programID, ringFixUpIndiceCount);

			// INTERIOR TRIM
			glew->bindVertexArray(interiorTrimVAO);
			//glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(1, 0, 0)[0]);
			model = glm::rotate(glm::mat4(1), glm::radians(rotAmounts[i]), glm::vec3(0.0f, 1.0f, 0.0f));
			glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &model[0][0]);
			glew->uniform1f(glew->getUniformLocation(programID, "scale"), 2 << i);
			Renderer::drawPartOfTerrain(glew, interiorTrimPositions[i].x, interiorTrimPositions[i].y, programID, interiorTrimIndiceCount);

			// OUTER DEGENERATE
			glew->bindVertexArray(outerDegenerateVAO);
			model = glm::mat4(1);
			//glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(1, 0, 1)[0]);
			glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &model[0][0]);
			glew->uniform1f(glew->getUniformLocation(programID, "scale"), 1 << i);
			Renderer::drawPartOfTerrain(glew, outerDegeneratePositions[i * 4 + 0].x, outerDegeneratePositions[i * 4 + 0].y, programID, outerDegenerateIndiceCount);
			Renderer::drawPartOfTerrain(glew, outerDegeneratePositions[i * 4 + 1].x, outerDegeneratePositions[i * 4 + 1].y, programID, outerDegenerateIndiceCount);
			model = glm::rotate(glm::mat4(1), glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f));
			glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &model[0][0]);
			Renderer::drawPartOfTerrain(glew, outerDegeneratePositions[i * 4 + 2].x, outerDegeneratePositions[i * 4 + 2].y, programID, outerDegenerateIndiceCount);
			Renderer::drawPartOfTerrain(glew, outerDegeneratePositions[i * 4 + 3].x, outerDegeneratePositions[i * 4 + 3].y, programID, outerDegenerateIndiceCount);
		}

		glew->bindVertexArray(blockVAO);
		glew->uniform1i(glew->getUniformLocation(programID, "level"), 0);
		glew->uniform1f(glew->getUniformLocation(programID, "scale"), 1);
		glew->uniform2f(glew->getUniformLocation(programID, "texturePos"), (float)terrain->initialTexturePositions[0].x, (float)terrain->initialTexturePositions[0].z);
		//glew->uniform2f(glew->getUniformLocation(programID, "texturePos"), (float)terrain->currentTileIndices[0].x * TILE_SIZE, (float)terrain->currentTileIndices[0].z * TILE_SIZE);
		glew->uniform1f(glew->getUniformLocation(programID, "texSize"), TILE_SIZE * (MEM_TILE_ONE_SIDE - 2));
		//glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 0, 1)[0]);
		glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &glm::mat4(1)[0][0]);
		for (int i = 0; i < 4; i++)
			Renderer::drawPartOfTerrain(glew, blockPositions[level * 12 + i].x, blockPositions[level * 12 + i].y, programID, blockIndiceCount);

		glew->bindVertexArray(ringFixUpVAO);
		//glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 1, 1)[0]);
		Renderer::drawPartOfTerrain(glew, ringFixUpPositions[level * 4 + 0].x, ringFixUpPositions[level * 4 + 0].y, programID, ringFixUpIndiceCount);
		Renderer::drawPartOfTerrain(glew, ringFixUpPositions[level * 4 + 2].x, ringFixUpPositions[level * 4 + 2].y, programID, ringFixUpIndiceCount);
		glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
		glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &model[0][0]);
		Renderer::drawPartOfTerrain(glew, ringFixUpPositions[level * 4 + 1].x, ringFixUpPositions[level * 4 + 1].y, programID, ringFixUpIndiceCount);
		Renderer::drawPartOfTerrain(glew, ringFixUpPositions[level * 4 + 3].x, ringFixUpPositions[level * 4 + 3].y, programID, ringFixUpIndiceCount);

		glew->bindVertexArray(smallSquareVAO);
		model = glm::mat4(1);
		glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &model[0][0]);
		//glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(1, 1, 1)[0]);
		Renderer::drawPartOfTerrain(glew, smallSquarePosition.x, smallSquarePosition.y, programID, smallSquareIndiceCount);

#endif 



		////std::cout << "Draw call count for the terrain: " << count << std::endl;
		//glew->bindVertexArray(0);
	}

	//void Renderer::drawBlock(glm::vec3& pos)

	//void Renderer::drawInternalPart(int programID, int blockVAO, int ringFixupVAO, int smallSquareVAO, int blockIndiceCount, int ringFixupIndiceCount,
	//	int smallSquareIndiceCount, int clipmapResolution, glm::vec3& camPos, glm::vec3& fakeDisplacement) {

	//	float posX = (int)(camPos.x / 2) * 2;
	//	float posZ = (int)(camPos.z / 2) * 2;

	//	GlewContext* glew = Core::instance->glewContext;
	//	glew->uniform1f(glew->getUniformLocation(programID, "scale"), 1);
	//	glew->uniform2f(glew->getUniformLocation(programID, "clipMapPos"), posX - fakeDisplacement.x + 1.f, posZ - fakeDisplacement.z + 1.f);
	//	glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 0, 1)[0]);
	//	glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &glm::mat4(1)[0][0]);
	//	glew->bindVertexArray(blockVAO);

	//	/*
	//	*  0 3
	//	*  1 2
	//	*/

	//	glm::vec3 position(2 - fakeDisplacement.x + posX, 0, 2 - fakeDisplacement.z + posZ);

	//	// 0
	//	Renderer::drawPartOfTerrain(glew, position.x, position.z, programID, blockIndiceCount);

	//	// 1
	//	position.z -= clipmapResolution + 1;
	//	Renderer::drawPartOfTerrain(glew, position.x, position.z, programID, blockIndiceCount);

	//	// 2
	//	position.x -= clipmapResolution + 1;
	//	Renderer::drawPartOfTerrain(glew, position.x, position.z, programID, blockIndiceCount);

	//	// 3
	//	position.z += clipmapResolution + 1;
	//	Renderer::drawPartOfTerrain(glew, position.x, position.z, programID, blockIndiceCount);

	//	// RING FIX-UP

	//		/*
	//		*    0
	//		*  1   3
	//		*    2
	//		*/

	//	glew->bindVertexArray(ringFixupVAO);
	//	glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 1, 1)[0]);

	//	position = glm::vec3(0 - fakeDisplacement.x + posX, 0, 2 - fakeDisplacement.z + posZ);

	//	// 0
	//	Renderer::drawPartOfTerrain(glew, position.x, position.z, programID, ringFixupIndiceCount);

	//	// 2
	//	position = glm::vec3(0 - fakeDisplacement.x + posX, 0, 1 - clipmapResolution - fakeDisplacement.z + posZ);
	//	Renderer::drawPartOfTerrain(glew, position.x, position.z, programID, ringFixupIndiceCount);

	//	glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
	//	glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &model[0][0]);

	//	// 1
	//	position = glm::vec3(2 - fakeDisplacement.x + posX, 0, 2 - fakeDisplacement.x + posZ);
	//	Renderer::drawPartOfTerrain(glew, position.x, position.z, programID, ringFixupIndiceCount);

	//	// 3
	//	position = glm::vec3(1 - clipmapResolution - fakeDisplacement.x + posX, 0, 2 - fakeDisplacement.x + posZ);
	//	Renderer::drawPartOfTerrain(glew, position.x, position.z, programID, ringFixupIndiceCount);

	//	// LAST SQUARE AT CENTER
	//	glew->bindVertexArray(smallSquareVAO);
	//	glew->uniform3fv(glew->getUniformLocation(programID, "color_d"), 1, &glm::vec3(1, 1, 1)[0]);

	//	model = glm::mat4(1);
	//	glew->uniformMatrix4fv(glew->getUniformLocation(programID, "model"), 1, 0, &model[0][0]);

	//	position = glm::vec3(0 - fakeDisplacement.x + posX, 0, 0 - fakeDisplacement.x + posZ);
	//	Renderer::drawPartOfTerrain(glew, position.x, position.z, programID, smallSquareIndiceCount);
	//}

	void Renderer::drawPartOfTerrain(GlewContext* glew, float x, float z, unsigned int programID, unsigned int indiceCount) {
		glew->uniform2f(glew->getUniformLocation(programID, "position"), x, z);
		glew->drawElements(0x0004, indiceCount, 0x1405, 0);
	}

	Entity* Renderer::detectAndGetEntityId(float mouseX, float mouseY) {

		Scene* scene = Core::instance->sceneManager->currentScene;
		if (!scene)
			return NULL;

		GlewContext* glew = Core::instance->glewContext;

		glew->bindFrameBuffer(0x8D40, Editor::instance->sceneCamera->FBO);
		glew->viewport(0, 0, (int)Editor::instance->menu->sceneRegion.x, (int)Editor::instance->menu->sceneRegion.y);
		glew->clearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glew->clear(0x00004000 | 0x00000100);
		glew->useProgram(pickingProgramID);

		SceneCamera* camera = Editor::instance->sceneCamera;
		glm::mat4& PV = camera->projectionViewMatrix;
		glm::vec3& camPos = camera->position;

		std::stack<Entity*> entStack;
		entStack.push(Core::instance->sceneManager->currentScene->root);

		while (!entStack.empty()) {

			Entity* popped = entStack.top();
			entStack.pop();

			for (Transform*& child : popped->transform->children)
				entStack.push(child->entity);

			MeshRenderer* renderer = popped->getComponent<MeshRenderer>();
			if (!renderer)
				continue;

			MeshFile* mesh = renderer->meshFile;
			MaterialFile* mat = renderer->materialFile;
			if (!mesh || !mat)
				continue;

			glm::mat4 model = popped->transform->model;
			glm::vec4 startInWorldSpace = model * mesh->aabbBox.start;
			glm::vec4 endInWorldSpace = model * mesh->aabbBox.end;

			if (!camera->intersectsAABB(startInWorldSpace, endInWorldSpace))
				continue;

			glew->uniformMatrix4fv(glew->getUniformLocation(pickingProgramID, "PV"), 1, 0, &PV[0][0]);
			glew->uniformMatrix4fv(glew->getUniformLocation(pickingProgramID, "model"), 1, 0, &model[0][0]);

			int r = (popped->id & 0x000000FF) >> 0;
			int g = (popped->id & 0x0000FF00) >> 8;
			int b = (popped->id & 0x00FF0000) >> 16;

			glew->uniform4f(glew->getUniformLocation(pickingProgramID, "pickingColor"), r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);

			glew->bindVertexArray(renderer->meshFile->VAO);
			glew->drawElements(0x0004, renderer->meshFile->indiceCount, 0x1405, (void*)0);
			glew->bindVertexArray(0);
		}

		/*for (int i = 0; i < Core::instance->sceneManager->currentScene->root->transform->children.size(); i++)
			Renderer::drawMeshRendererForPickingRecursively(Core::instance->sceneManager->currentScene->root->transform->children[i]->entity);*/

		// Wait until all the pending drawing commands are really done.
		// Ultra-mega-over slow ! 
		// There are usually a long time between glDrawElements() and
		// all the fragments completely rasterized.
		glew->flush();
		glew->finish();

		glew->pixelStorei(0x0CF5, 1);

		// Read the pixel at the center of the screen.
		// You can also use glfwGetMousePos().
		// Ultra-mega-over slow too, even for 1 pixel, 
		// because the framebuffer is on the GPU.
		unsigned char data[4];
		glew->readPixels(mouseX, mouseY, 1, 1, 0x1908, 0x1401, data);

		// Convert the color back to an integer ID
		int pickedID =
			data[0] +
			data[1] * 256 +
			data[2] * 256 * 256;

		//if (pickedID == 0x00ffffff || pickedID >= Editor::instance->scene->entities.size())
		//	Editor::instance->editorGUI.lastSelectedEntity = NULL;
		//else
		//	Editor::instance->editorGUI.lastSelectedEntity = Editor::instance->scene->entities[pickedID];

		glew->bindFrameBuffer(0x8D40, 0);

		if (Core::instance->sceneManager->currentScene->entityIdToEntity.find(pickedID) != Core::instance->sceneManager->currentScene->entityIdToEntity.end()) {
			return Core::instance->sceneManager->currentScene->entityIdToEntity[pickedID];
		}
		return NULL;

		//if (pickedID == 0x00ffffff || pickedID > Core::instance->scene->idCounter)
		//else
		//	return pickedID;
	}

	void Renderer::drawMeshRendererForPickingRecursively(Entity* entity) {

		MeshRenderer* renderer = entity->getComponent<MeshRenderer>();
		glm::mat4 model = entity->transform->model;
		glm::mat4& PV = Editor::instance->sceneCamera->projectionViewMatrix;
		GlewContext* glew = Core::instance->glewContext;

		if (renderer != NULL) {
			if (renderer->meshFile != NULL) {

				glew->useProgram(pickingProgramID);
				glew->uniformMatrix4fv(glew->getUniformLocation(pickingProgramID, "PV"), 1, 0, &PV[0][0]);
				glew->uniformMatrix4fv(glew->getUniformLocation(pickingProgramID, "model"), 1, 0, &model[0][0]);

				int r = (entity->id & 0x000000FF) >> 0;
				int g = (entity->id & 0x0000FF00) >> 8;
				int b = (entity->id & 0x00FF0000) >> 16;

				glew->uniform4f(glew->getUniformLocation(pickingProgramID, "pickingColor"), r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);

				glew->bindVertexArray(renderer->meshFile->VAO);
				glew->drawElements(0x0004, renderer->meshFile->indiceCount, 0x1405, (void*)0);
				glew->bindVertexArray(0);
			}
		}

		for (auto& transform : entity->transform->children)
			Renderer::drawMeshRendererForPickingRecursively(transform->entity);
	}
}