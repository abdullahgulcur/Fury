#include "pch.h"
#include "renderer.h"
#include "editor.h"
#include "scenecamera.h"
#include "entity.h"
#include "scene.h"

namespace Editor {

	Renderer::Renderer() {

	}

	void Renderer::init() {

		GlewContext* glew= Core::instance->glewContext;

		Renderer::initDefaultSphere();

		pickingProgramID = Core::instance->glewContext->loadShaders("C:/Projects/Fury/Editor/src/shader/ObjectPick.vert",
			"C:/Projects/Fury/Editor/src/shader/ObjectPick.frag");

		//framebufferProgramID = Core::instance->glewContext->loadShaders("C:/Projects/Fury/Editor/src/shader/framebuffer.vert",
		//	"C:/Projects/Fury/Editor/src/shader/framebuffer.frag");


		//float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		//	// positions   // texCoords
		//	-1.0f,  1.0f,  0.0f, 1.0f,
		//	-1.0f, -1.0f,  0.0f, 0.0f,
		//	 1.0f, -1.0f,  1.0f, 0.0f,

		//	-1.0f,  1.0f,  0.0f, 1.0f,
		//	 1.0f, -1.0f,  1.0f, 0.0f,
		//	 1.0f,  1.0f,  1.0f, 1.0f
		//};
		//// screen quad VAO
		//unsigned int quadVAO, quadVBO;
		//glew->genVertexArrays(1, &quadVAO);
		//glew->genBuffers(1, &quadVBO);
		//glew->bindVertexArray(quadVAO);
		//glew->bindBuffer(0x8892, quadVBO);
		//glew->bufferData(0x8892, sizeof(quadVertices), &quadVertices, 0x88E4);
		//glew->enableVertexAttribArray(0);
		//glew->vertexAttribPointer(0, 2, 0x1406, 0, 4 * sizeof(float), (void*)0);
		//glew->enableVertexAttribArray(1);
		//glew->vertexAttribPointer(1, 2, 0x1406, 0, 4 * sizeof(float), (void*)(2 * sizeof(float)));
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

		if (Core::instance->sceneManager->currentScene) {

			glm::mat4& VP = Editor::instance->sceneCamera->projectionViewMatrix;

			Core::instance->glewContext->bindFrameBuffer(Editor::instance->sceneCamera->FBO);
			Core::instance->glewContext->viewport(Editor::instance->menu->sceneRegion.x, Editor::instance->menu->sceneRegion.y);
			Core::instance->glewContext->clearScreen(glm::vec3(0.3f, 0.3f, 0.3f));

			for (int i = 0; i < Core::instance->sceneManager->currentScene->root->transform->children.size(); i++)
				Renderer::drawMeshRendererRecursively(Core::instance->sceneManager->currentScene->root->transform->children[i]->entity,
					Editor::instance->sceneCamera->projectionViewMatrix, Editor::instance->sceneCamera->position);

			Core::instance->glewContext->bindFrameBuffer(0);
		}


		//if (Core::instance->scene && Core::instance->scene->primaryCamera) {

		//	glm::mat4& VP = Core::instance->scene->primaryCamera->projectionViewMatrix;

		//	Core::instance->glewContext->bindFrameBuffer(Core::instance->scene->primaryCamera->FBO);

		//	Core::instance->glewContext->viewport(Editor::instance->menu->gameRegion.x, Editor::instance->menu->gameRegion.y);

		//	Core::instance->glewContext->clearScreen(glm::vec3(0.3f, 0.3f, 0.3f));

		//	for (int i = 0; i < Core::instance->scene->root->transform->children.size(); i++)
		//		Renderer::drawMeshRendererRecursively(Core::instance->scene->root->transform->children[i]->entity,
		//			Core::instance->scene->primaryCamera->projectionViewMatrix, Core::instance->scene->primaryCamera->position);

		//	Core::instance->glewContext->bindFrameBuffer(0);
		//}
	}

	void Renderer::drawMeshRendererRecursively(Entity* entity, glm::mat4& PV, glm::vec3& camPos) {

		MeshRenderer* renderer = entity->getComponent<MeshRenderer>();
		Terrain* terrain = entity->getComponent<Terrain>();
		GameCamera* gamecamera = entity->getComponent<GameCamera>();
		glm::mat4 model = entity->transform->model;
		glm::mat4& VP = PV;
		GlewContext* glew = Core::instance->glewContext;

		if (renderer != NULL) {
			if (renderer->meshFile && renderer->materialFile) {

				unsigned int programId = renderer->materialFile->programId;
				glew->useProgram(programId);
				glew->uniform3fv(glew->getUniformLocation(programId, "camPos"), 1, &camPos[0]);
				glew->uniformMatrix4fv(glew->getUniformLocation(programId, "PV"), 1, 0, &VP[0][0]);
				glew->uniformMatrix4fv(glew->getUniformLocation(programId, "model"), 1, 0, &model[0][0]);

				if (renderer->materialFile->shaderTypeId == 0) {

					for (int i = 0; i < renderer->materialFile->activeTextureIndices.size(); i++) {

						std::string texStr = "texture" + std::to_string(renderer->materialFile->activeTextureIndices[i]);
						glew->activeTexture(0x84C0 + i);
						glew->bindTexture(0x0DE1, renderer->materialFile->textureFiles[i]->textureId);
						glew->uniform1i(glew->getUniformLocation(programId, &texStr[0]), i);
					}	
				}

				glew->bindVertexArray(renderer->meshFile->VAO);
				glew->drawElements(0x0004, renderer->meshFile->indiceCount, 0x1405, (void*)0);
				glew->bindVertexArray(0);
			}
		}

		if (terrain != NULL)
			Renderer::drawTerrain(Editor::instance->sceneCamera, terrain);

		if (gamecamera != NULL && Editor::instance->menu->selectedEntity == entity) // bunu ayir
			gamecamera->drawEditorGizmos(PV, entity->transform->model);

		for (auto& transform : entity->transform->children)
			Renderer::drawMeshRendererRecursively(transform->entity, PV, camPos);
	}


	void Renderer::drawTerrain(SceneCamera* camera, Terrain* terrain) { // GameCamera* gc, , Terrain* terrain

		GlewContext* glew = Core::instance->glewContext;

		int count = 0;
		float scale = terrain->triangleSize;
		int level = terrain->clipmapLevel;
		int clipmapResolution = terrain->clipmapResolution;
		int worldSize = scale * clipmapResolution * (2 << level);
		unsigned int elevationMapSize = terrain->elevationMapSize;
		int programID = terrain->programID;
		float* pvAddr = &camera->projectionViewMatrix[0][0];

		// Change this for debugging purposes--------
		glm::vec3 camPos = camera->position;
		//glm::vec3 camPos = gc->getPosition();
		//-------------------------------------------

		float* lightDirAddr = &glm::normalize(-glm::vec3(1, -0.35f, -1))[0];
		float* lightColAddr = &glm::vec3(1, 1, 1)[0];

		int elevationMapTexture = terrain->elevationMapTexture;
		int normalMapTexture = terrain->normalMapTexture;

		glew->useProgram(programID);
		glew->uniformMatrix4fv(glew->getUniformLocation(programID, "PV"), 1, 0, pvAddr);

		glew->uniform1i(glew->getUniformLocation(programID, "patchRes"), level);
		glew->uniform1i(glew->getUniformLocation(programID, "mapSize"), elevationMapSize);
		glew->uniform1i(glew->getUniformLocation(programID, "clipMapSize"), worldSize);
		glew->uniform3fv(glew->getUniformLocation(programID, "camPos"), 1, &camPos[0]);
		glew->uniform3fv(glew->getUniformLocation(programID, "lightDir"), 1, lightDirAddr);
		glew->uniform3fv(glew->getUniformLocation(programID, "lightColor"), 1, lightColAddr);
		glew->uniform1f(glew->getUniformLocation(programID, "triSize"), scale);

		glew->activeTexture(0x84C0);
		glew->bindTexture(0x0DE1, elevationMapTexture);
		glew->uniform1i(glew->getUniformLocation(programID, "heightmap"), 0);

		glew->activeTexture(0x84C1);
		glew->bindTexture(0x0DE1, normalMapTexture);
		glew->uniform1i(glew->getUniformLocation(programID, "normalmap"), 1);

		//glm::vec2 camDistBoundaries;
		//camDistBoundaries.x = worldSize * 0.51f; // 0.51 cok gecici bir cozum...
		//camDistBoundaries.y = elevationMapSize - worldSize * 0.51f;
		//camPos.x = std::clamp(camPos.x, camDistBoundaries.x, camDistBoundaries.y);
		//camPos.z = std::clamp(camPos.z, camDistBoundaries.x, camDistBoundaries.y);

		float amount = 2.f * scale;

		for (int i = 0; i < level; i++) {

			float ox = int(camPos.x / (amount * (1 << i))) * (2.f / clipmapResolution);
			float oz = int(camPos.z / (amount * (1 << i))) * (2.f / clipmapResolution);
			float x = -((int)(ox * (clipmapResolution / 2.f)) % 2) * 4;
			float z = -((int)(oz * (clipmapResolution / 2.f)) % 2) * 4;
			float ax = -((int)(ox * (clipmapResolution / 2.f)) % 2) * (2.f / clipmapResolution);
			float az = -((int)(oz * (clipmapResolution / 2.f)) % 2) * (2.f / clipmapResolution);

			for (int j = -2; j < 2; j++) {

				for (int k = -2; k < 2; k++) {

					if (i != 0) if (k == -1 || k == 0) if (j == -1 || j == 0) continue;

					glm::vec2 blockOffset(j, k);
					if (j == -1 || j == 1) blockOffset.x -= 1.f / clipmapResolution;
					if (k == -1 || k == 1) blockOffset.y -= 1.f / clipmapResolution;

					blockOffset.x += 2.f / clipmapResolution;
					blockOffset.y += 2.f / clipmapResolution;

					glm::vec3 off((blockOffset.x + ox) * clipmapResolution, 0, (blockOffset.y + oz) * clipmapResolution);

					glm::vec3 start = off * scale;
					start.y = 0;
					glm::vec3 end = off * scale + scale * glm::vec3(clipmapResolution - 1, 0, clipmapResolution - 1);
					end.y = 0;
					//terrain->setBoundariesOfClipmap(i, start, end);
					//Terrain::drawTerrainClipmapAABB(start, end, camera, editor);

					glew->useProgram(programID);
					glew->uniform1f(glew->getUniformLocation(programID, "scale"), scale);

					//gc->intersectsAABB(start, end)
					if (camera->intersectsAABB(start, end)) {

						glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
						//glUniform3fv(glGetUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 0, 1)[0]);
						glew->bindVertexArray(terrain->blockVAO);
						glew->drawElements(0x0004, terrain->blockIndices.size(), 0x1405, 0);
						count++;
					}

					if (j == 0) {

						off = glm::vec3((blockOffset.x - 2.f / clipmapResolution + ox) * clipmapResolution, 0, (blockOffset.y + oz) * clipmapResolution);

						glm::vec3 start = off * scale;
						start.y = 0;
						glm::vec3 end = off * scale + scale * glm::vec3(2, 0, clipmapResolution - 1);
						end.y = 0;
						//terrain->setBoundariesOfClipmap(0, start, end);
						//Terrain::drawTerrainClipmapAABB(start, end, camera, editor);

						if (camera->intersectsAABB(start, end)) {

							glew->useProgram(programID);
							glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
							//glUniform3fv(glGetUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 1, 0)[0]);
							glew->bindVertexArray(terrain->ringFixUpHorizontalVAO);
							glew->drawElements(0x0004, terrain->ringFixUpHorizontalIndices.size(), 0x1405, 0);
							count++;
						}



						if (i == 0 && k == 0) {

							off.z -= 2;

							glm::vec3 start = off * scale;
							start.y = 0;
							glm::vec3 end = off * scale + scale * glm::vec3(2, 0, 2);
							end.y = 0;
							//terrain->setBoundariesOfClipmap(0, start, end);
							//Terrain::drawTerrainClipmapAABB(start, end, i, camera, editor);

							if (camera->intersectsAABB(start, end)) {

								glew->useProgram(programID);
								glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
								glew->bindVertexArray(terrain->smallSquareVAO);
								glew->drawElements(0x0004, terrain->smallSquareIndices.size(), 0x1405, 0);
								count++;
							}


						}
					}
					else if (j == 1) {

						off = glm::vec3((blockOffset.x + 1 - 1.f / clipmapResolution + ox + x) * clipmapResolution, 0, (blockOffset.y + oz) * clipmapResolution);

						glm::vec3 start = off * scale;
						start.y = 0;
						glm::vec3 end = off * scale + scale * glm::vec3(2, 0, clipmapResolution - 1);
						end.y = 0;
						//terrain->setBoundariesOfClipmap(0, start, end);
						//Terrain::drawTerrainClipmapAABB(start, end, camera, editor);

						if (camera->intersectsAABB(start, end)) {

							glew->useProgram(programID);
							glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
							//glUniform3fv(glGetUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 1, 1)[0]);
							glew->bindVertexArray(terrain->ringFixUpHorizontalVAO);
							glew->drawElements(0x0004, terrain->ringFixUpHorizontalIndices.size(), 0x1405, 0);
							count++;
						}

						if (k == 0) {

							off.z -= 2;

							glm::vec3 start = off * scale;
							start.y = 0;
							glm::vec3 end = off * scale + scale * glm::vec3(2, 0, 2);
							end.y = 0;
							//terrain->setBoundariesOfClipmap(0, start, end);
							//Terrain::drawTerrainClipmapAABB(start, end, i, camera, editor);

							if (camera->intersectsAABB(start, end)) {

								glew->useProgram(programID);
								glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
								glew->bindVertexArray(terrain->smallSquareVAO);
								glew->drawElements(0x0004, terrain->smallSquareIndices.size(), 0x1405, 0);
								count++;
							}

						}
					}

					if (k == 1) {

						off = glm::vec3((blockOffset.x + ox) * clipmapResolution, 0, (blockOffset.y + 1 - 1.f / clipmapResolution + oz + z) * clipmapResolution);

						glm::vec3 start = off * scale;
						start.y = 0;
						glm::vec3 end = off * scale + scale * glm::vec3(clipmapResolution - 1, 0, 2);
						end.y = 0;
						//terrain->setBoundariesOfClipmap(0, start, end);
						//Terrain::drawTerrainClipmapAABB(start, end, camera, editor);

						if (camera->intersectsAABB(start, end)) {

							glew->useProgram(programID);
							glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
							//glUniform3fv(glGetUniformLocation(programID, "color_d"), 1, &glm::vec3(0, 1, 1)[0]);
							glew->bindVertexArray(terrain->ringFixUpVerticalVAO);
							glew->drawElements(0x0004, terrain->ringFixUpVerticalIndices.size(), 0x1405, 0);
							count++;
						}


						if (j == 0) {

							off.x -= 2;

							glm::vec3 start = off * scale;
							start.y = 0;
							glm::vec3 end = off * scale + scale * glm::vec3(2, 0, 2);
							end.y = 0;
							//terrain->setBoundariesOfClipmap(0, start, end);
							//Terrain::drawTerrainClipmapAABB(start, end, i, camera, editor);

							if (camera->intersectsAABB(start, end)) {

								glew->useProgram(programID);
								glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
								glew->bindVertexArray(terrain->smallSquareVAO);
								glew->drawElements(0x0004, terrain->smallSquareIndices.size(), 0x1405, 0);
								count++;
							}
						}
					}
					else if (k == 0) {

						off = glm::vec3((blockOffset.x + ox) * clipmapResolution, 0, (blockOffset.y - 2.f / clipmapResolution + oz) * clipmapResolution);

						glm::vec3 start = off * scale;
						start.y = 0;
						glm::vec3 end = off * scale + scale * glm::vec3(clipmapResolution - 1, 0, 2);
						end.y = 0;
						//terrain->setBoundariesOfClipmap(0, start, end);
						//Terrain::drawTerrainClipmapAABB(start, end, camera, editor);

						if (camera->intersectsAABB(start, end)) {

							glew->useProgram(programID);
							glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
							//glUniform3fv(glGetUniformLocation(programID, "color_d"), 1, &glm::vec3(1, 0.5, 0)[0]);
							glew->bindVertexArray(terrain->ringFixUpVerticalVAO);
							glew->drawElements(0x0004, terrain->ringFixUpVerticalIndices.size(), 0x1405, 0);
							count++;
						}
					}

					if (j == 1 && k == 1) {

						off = glm::vec3((blockOffset.x + 1 - 1.f / clipmapResolution + ox + x) * clipmapResolution, 0, (blockOffset.y + 1 - 1.f / clipmapResolution + oz + z) * clipmapResolution);

						glm::vec3 start = off * scale;
						start.y = 0;
						glm::vec3 end = off * scale + scale * glm::vec3(2, 0, 2);
						end.y = 0;
						//terrain->setBoundariesOfClipmap(0, start, end);
						//Terrain::drawTerrainClipmapAABB(start, end, i, camera, editor);

						if (camera->intersectsAABB(start, end)) {

							glew->useProgram(programID);
							glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
							//glUniform3fv(glGetUniformLocation(programID, "color_d"), 1, &glm::vec3(1, 1, 1)[0]);
							glew->bindVertexArray(terrain->smallSquareVAO);
							glew->drawElements(0x0004, terrain->smallSquareIndices.size(), 0x1405, 0);
							count++;
						}

					}

					if (j == -2 && k == -2) {

						off = glm::vec3((blockOffset.x + ox + ax) * clipmapResolution, 0, (blockOffset.y + oz + az) * clipmapResolution);
						glew->uniform3fv(glew->getUniformLocation(programID, "offsetMultPatchRes"), 1, &off[0]);
						//glUniform3fv(glGetUniformLocation(programID, "color_d"), 1, &glm::vec3(1, 0, 0)[0]);
						glew->bindVertexArray(terrain->outerDegenerateVAO);
						glew->drawElements(0x0004, terrain->outerDegenerateIndices.size(), 0x1405, 0);
						count++;
					}
				}
			}
			scale *= 2;
		}

		//std::cout << "Draw call count for the terrain: " << count << std::endl;
		glew->bindVertexArray(0);


	}

	Entity* Renderer::detectAndGetEntityId(float mouseX, float mouseY) {

		GlewContext* glew = Core::instance->glewContext;

		glew->bindFrameBuffer(0x8D40, Editor::instance->sceneCamera->FBO);
		glew->viewport(0, 0, (int)Editor::instance->menu->sceneRegion.x, (int)Editor::instance->menu->sceneRegion.y);
		glew->clearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glew->clear(0x00004000 | 0x00000100);
		glew->useProgram(pickingProgramID);

		for (int i = 0; i < Core::instance->sceneManager->currentScene->root->transform->children.size(); i++)
			Renderer::drawMeshRendererForPickingRecursively(Core::instance->sceneManager->currentScene->root->transform->children[i]->entity);

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