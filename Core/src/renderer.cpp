#include "pch.h"
#include "renderer.h"
#include "scene.h"
#include "core.h"
#include "entity.h"

namespace Fury {

    Core* Core::instance;

    Renderer::Renderer() {

    }

    void Renderer::init() {

        width = Core::instance->glfwContext->mode->width;
        height = Core::instance->glfwContext->mode->height;

        GlewContext* glew = Core::instance->glewContext;

        Renderer::initDefaultSphere();

        framebufferProgramID = Core::instance->glewContext->loadShaders("C:/Projects/Fury/Core/src/shader/framebuffer.vert",
            "C:/Projects/Fury/Core/src/shader/framebuffer.frag");

        float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };
        // screen quad VAO
        unsigned int quadVBO;
        glew->genVertexArrays(1, &quadVAO);
        glew->genBuffers(1, &quadVBO);
        glew->bindVertexArray(quadVAO);
        glew->bindBuffer(0x8892, quadVBO);
        glew->bufferData(0x8892, sizeof(quadVertices), &quadVertices, 0x88E4);
        glew->enableVertexAttribArray(0);
        glew->vertexAttribPointer(0, 2, 0x1406, 0, 4 * sizeof(float), (void*)0);
        glew->enableVertexAttribArray(1);
        glew->vertexAttribPointer(1, 2, 0x1406, 0, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }

    void Renderer::update() {

        /* reset */
        drawCallCount = 0;

        Scene* scene = Core::instance->sceneManager->currentScene;
        GameCamera* camera = scene->primaryCamera;

        if (scene && camera) {

            GlewContext* glew = Core::instance->glewContext;

	        glm::mat4& VP = camera->projectionViewMatrix;
            glm::vec3& camPos = camera->position;

	        Core::instance->glewContext->bindFrameBuffer(camera->FBO);
            glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
	        Core::instance->glewContext->viewport(camera->width, camera->height);
	        Core::instance->glewContext->clearScreen(glm::vec3(0.3f, 0.3f, 0.3f));

	        //for (int i = 0; i < Core::instance->sceneManager->currentScene->root->transform->children.size(); i++)
		       // Renderer::drawMeshRendererRecursively(Core::instance->sceneManager->currentScene->root->transform->children[i]->entity,
			      //  Core::instance->sceneManager->currentScene->primaryCamera->projectionViewMatrix, Core::instance->sceneManager->currentScene->primaryCamera->position);

           

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
                if(!mesh || !mat)
                    continue;

                glm::mat4 model = popped->transform->model;
                glm::vec4 startInWorldSpace = model * mesh->aabbBox.start;
                glm::vec4 endInWorldSpace = model * mesh->aabbBox.end;

                if (!camera->intersectsAABB(startInWorldSpace, endInWorldSpace))
                    continue;

                unsigned int programId = mat->programId;
                glew->useProgram(programId);
                glew->uniform3fv(glew->getUniformLocation(programId, "camPos"), 1, &camPos[0]);
                glew->uniformMatrix4fv(glew->getUniformLocation(programId, "PV"), 1, 0, &VP[0][0]);
                glew->uniformMatrix4fv(glew->getUniformLocation(programId, "model"), 1, 0, &model[0][0]);

                if (mat->shaderTypeId == 0) {

                    for (int i = 0; i < mat->activeTextureIndices.size(); i++) {

                        std::string texStr = "texture" + std::to_string(mat->activeTextureIndices[i]);
                        glew->activeTexture(0x84C0 + i);
                        glew->bindTexture(0x0DE1, mat->textureFiles[i]->textureId);
                        glew->uniform1i(glew->getUniformLocation(programId, &texStr[0]), i);
                    }
                }

                glew->bindVertexArray(mesh->VAO);
                glew->drawElements(0x0004, mesh->indiceCount, 0x1405, (void*)0);
                glew->bindVertexArray(0);

                drawCallCount++;
            }

	        Core::instance->glewContext->bindFrameBuffer(0);

#ifndef EDITOR_MODE
            Core::instance->glewContext->clearColor(1.0f, 1.0f, 1.0f, 1.0f);// set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
            glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
            Core::instance->glewContext->clear(0x00004000);
            Core::instance->glewContext->useProgram(framebufferProgramID);
            Core::instance->glewContext->bindVertexArray(quadVAO);
            //Core::instance->glewContext->bindTexture(0x0DE1, Core::instance->scene->primaryCamera->textureBuffer);	// use the color attachment texture as the texture of the quad plane

            Core::instance->glewContext->activeTexture(0x84C0);
            Core::instance->glewContext->bindTexture(0x0DE1, Core::instance->sceneManager->currentScene->primaryCamera->textureBuffer);	// use the color attachment texture as the texture of the quad plane
            Core::instance->glewContext->uniform1i(Core::instance->glewContext->getUniformLocation(framebufferProgramID, "screenTexture"), 0);
            Core::instance->glewContext->drawArrays(0x0004, 0, 6);
#endif // EDITOR_MODE
        }
    }

    void Renderer::drawMeshRendererRecursively(Entity* entity, glm::mat4& PV, glm::vec3& camPos) {

        MeshRenderer* renderer = entity->getComponent<MeshRenderer>();
        //Terrain* terrain = entity->getComponent<Terrain>();
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

        //if (terrain != NULL)
        //    Renderer::drawTerrain(Editor::instance->sceneCamera, terrain);

        //if (gamecamera != NULL && Editor::instance->menu->selectedEntity == entity) // bunu ayir
        //    gamecamera->drawEditorGizmos(PV, entity->transform->model);

        for (auto& transform : entity->transform->children)
            Renderer::drawMeshRendererRecursively(transform->entity, PV, camPos);
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


}