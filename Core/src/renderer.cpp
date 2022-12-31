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
        //Renderer::initMaterialFileTextures();
        //Renderer::initMeshFileTextures();

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

        //// framebuffer configuration
        //// -------------------------
        //glew->genFramebuffers(1, &FBO);
        //glew->bindFramebuffer(0x8D40, FBO);
        //// create a color attachment texture
        //unsigned int textureColorbuffer;
        //glew->genTextures(1, &textureColorbuffer);
        //glew->bindTexture(0x0DE1, textureColorbuffer);
        //glew->texImage2D(0x0DE1, 0, 0x1907, Core::instance->glfwContext->mode->width, Core::instance->glfwContext->mode->height, 0, 0x1907, 0x1401, NULL);
        //glew->texParameteri(0x0DE1, 0x2801, 0x2601);
        //glew->texParameteri(0x0DE1, 0x2800, 0x2601);
        //glew->framebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, textureColorbuffer, 0);
        //// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        //unsigned int rbo;
        //glew->genRenderbuffers(1, &rbo);
        //glew->bindRenderbuffer(0x8D41, rbo);
        //glew->renderbufferStorage(0x8D41, 0x88F0, Core::instance->glfwContext->mode->width, Core::instance->glfwContext->mode->height); // use a single renderbuffer object for both a depth AND stencil buffer.
        //glew->framebufferRenderbuffer(0x8D40, 0x821A, 0x8D41, rbo); // now actually attach it
        //// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
        //if (glew->checkFramebufferStatus(0x8D40) != 0x8CD5)
        //    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        //glew->bindFramebuffer(0x8D40, 0);
    }

    //void Renderer::initMaterialFileTextures() {

    //    //glm::vec3 camPos(0, 0, -2.1f);
    //    //glm::mat4 view = glm::lookAt(camPos, camPos + glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
    //    //glm::mat4 projection = glm::perspective(glm::radians(60.f), 1.f, 0.01f, 100.f);

    //    //GlewContext* glewContext = editor->core->glewContext;

    //    //for (auto& file : editor->core->fileSystem->matFiles) {

    //    //    MaterialFile* matFile = editor->core->fileSystem->fileToMatFile[file];
    //    //    glewContext->bindFrameBuffer(matFile->FBO);
    //    //    glewContext->viewport(64, 64);
    //    //    glewContext->clearScreen(glm::vec3(0.1f, 0.1f, 0.1f));

    //    //    glewContext->useProgram(matFile->programId);
    //    //    glewContext->setVec3(matFile->programId, "camPos", camPos);
    //    //    glewContext->setMat4(matFile->programId, "PV", projection * view);
    //    //    glewContext->setMat4(matFile->programId, "model", glm::mat4(1));

    //    //    for (int i = 0; i < matFile->activeTextureIndices.size(); i++) {
    //    //        glewContext->activeTex(i);
    //    //        glewContext->bindTex(matFile->textureFiles[i]->textureId);
    //    //        glewContext->setInt(matFile->programId, "texture" + std::to_string(matFile->activeTextureIndices[i]), i);
    //    //    }

    //    //    glewContext->bindVertexArray(defaultSphereVAO);
    //    //    glewContext->drawElements_triStrip(defaultSphereIndexCount);
    //    //    glewContext->bindVertexArray(0);

    //    //    file->textureID = matFile->fileTextureId;
    //    //    editor->core->glewContext->bindFrameBuffer(0);
    //    //}
    //}

    //void Renderer::initMeshFileTextures() {

    //    //glm::mat4 projection = glm::perspective(glm::radians(60.f), 1.f, 0.01f, 100.f);
    //    //GlewContext* glewContext = core->glewContext;

    //    //for (auto& file : core->fileSystem->meshFiles) {

    //    //    MeshFile* meshFile = core->fileSystem->fileToMeshFile[file];
    //    //    float z = (meshFile->radius * 1.2f);// *glm::sin(3.14f / 4);
    //    //    float y = (meshFile->radius * 1.5f) * glm::sin(3.14f / 4);
    //    //    float x = (meshFile->radius * 1.5f) * glm::cos(3.14f / 4);
    //    //    glm::vec3 camPos(z, -z, z);
    //    //    glm::mat4 view = glm::lookAt(camPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    //    //    glewContext->bindFrameBuffer(meshFile->FBO);
    //    //    glewContext->viewport(64, 64);
    //    //    glewContext->clearScreen(glm::vec3(0.12f, 0.12f, 0.12f));
    //    //    //glewContext->lineWidth(0.15f);
    //    //   /* glewContext->useProgram(editor->gizmo->wireframeRendererProgramID);
    //    //    glewContext->setMat4(editor->gizmo->wireframeRendererProgramID, "PV", projection * view);
    //    //    glewContext->setMat4(editor->gizmo->wireframeRendererProgramID, "model", glm::mat4(1));
    //    //    glewContext->setVec3(editor->gizmo->wireframeRendererProgramID, "color", glm::vec3(0.25f, 0.25f, 0.25f));
    //    //    glewContext->bindVertexArray(meshFile->wireframeVAO);
    //    //    glewContext->drawElements(0x0001, meshFile->wireframeIndiceCount, 0x1405, (void*)0);*/
    //    //    unsigned int programId = core->fileSystem->pbrMaterialNoTexture->programId;
    //    //    glewContext->useProgram(programId);
    //    //    glewContext->setMat4(programId, "PV", projection * view);
    //    //    glewContext->setMat4(programId, "model", glm::mat4(1));
    //    //    glewContext->setVec3(programId, "camPos", camPos);
    //    //    glewContext->bindVertexArray(meshFile->VAO);
    //    //    glewContext->drawElements(0x0004, meshFile->indiceCount, 0x1405, (void*)0);

    //    //    glewContext->bindVertexArray(0);

    //    //    file->textureID = meshFile->fileTextureId;
    //    //    core->glewContext->bindFrameBuffer(0);
    //    //}
    //}

    void Renderer::update() {



        if (Core::instance->sceneManager->currentScene && Core::instance->sceneManager->currentScene->primaryCamera) {

	        glm::mat4& VP = Core::instance->sceneManager->currentScene->primaryCamera->projectionViewMatrix;

	        Core::instance->glewContext->bindFrameBuffer(Core::instance->sceneManager->currentScene->primaryCamera->FBO);
            glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

	        Core::instance->glewContext->viewport(Core::instance->sceneManager->currentScene->primaryCamera->width, Core::instance->sceneManager->currentScene->primaryCamera->height);

	        Core::instance->glewContext->clearScreen(glm::vec3(0.3f, 0.3f, 0.3f));

	        for (int i = 0; i < Core::instance->sceneManager->currentScene->root->transform->children.size(); i++)
		        Renderer::drawMeshRendererRecursively(Core::instance->sceneManager->currentScene->root->transform->children[i]->entity,
			        Core::instance->sceneManager->currentScene->primaryCamera->projectionViewMatrix, Core::instance->sceneManager->currentScene->primaryCamera->position);

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

//#ifdef EDITOR_MODE
//        if (Core::instance->scene) {
//
//            glm::mat4& VP = Editor::instance->sceneCamera->projectionViewMatrix;
//
//            Core::instance->glewContext->bindFrameBuffer(Editor::instance->sceneCamera->FBO);
//            Core::instance->glewContext->viewport(Editor::instance->menu->sceneRegion.x, Editor::instance->menu->sceneRegion.y);
//            Core::instance->glewContext->clearScreen(glm::vec3(0.3f, 0.3f, 0.3f));
//
//            for (int i = 0; i < Core::instance->scene->root->transform->children.size(); i++)
//                Renderer::drawMeshRendererRecursively(Core::instance->scene->root->transform->children[i]->entity,
//                    Editor::instance->sceneCamera->projectionViewMatrix, Editor::instance->sceneCamera->position);
//
//            Core::instance->glewContext->bindFrameBuffer(0);
//        }
//#endif // EDITOR_MODE
//
//
//        if (Core::instance->scene && Core::instance->scene->primaryCamera) {
//
//            glm::mat4& VP = Core::instance->scene->primaryCamera->projectionViewMatrix;
//
//            Core::instance->glewContext->bindFrameBuffer(Core::instance->scene->primaryCamera->FBO);
//
//#ifdef EDITOR_MODE
//            Core::instance->glewContext->viewport(Editor::instance->menu->gameRegion.x, Editor::instance->menu->gameRegion.y);
//#else
//            Core::instance->glewContext->viewport(Core::instance->glfwContext->mode->width, Core::instance->glfwContext->mode->height);
//#endif // EDITOR_MODE
//
//            Core::instance->glewContext->clearScreen(glm::vec3(0.3f, 0.3f, 0.3f));
//
//            for (int i = 0; i < Core::instance->scene->root->transform->children.size(); i++)
//                Renderer::drawMeshRendererRecursively(Core::instance->scene->root->transform->children[i]->entity,
//                    Core::instance->scene->primaryCamera->projectionViewMatrix, Core::instance->scene->primaryCamera->position);
//
//            Core::instance->glewContext->bindFrameBuffer(0);
//

//
//        }
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

    //void Renderer::update() {

    //    glm::mat4& VP = editor->sceneCamera->projectionViewMatrix;

    //    editor->core->glewContext->bindFrameBuffer(editor->sceneCamera->FBO);
    //    editor->core->glewContext->viewport(editor->menu->sceneRegion.x, editor->menu->sceneRegion.y);
    //    editor->core->glewContext->clearScreen(glm::vec3(0.4f, 0.65f, 0.8f));
    //    // draw whole the fucking shit...
    //    //editor->core->glewContext->drawLineVAO(editor->gizmo->lineRendererProgramID, editor->gizmo->cubeVAO, VP);

    //    for (int i = 0; i < editor->core->scene->root->transform->children.size(); i++) {
    //        MeshRenderer* renderer = editor->core->scene->root->transform->children[i]->entity->getComponent<MeshRenderer>();

    //        if (renderer != NULL) {

    //            if (renderer->meshFile != NULL) {


    //                // (4) call updated draw mesh with mesh renderer component...

    //                if (renderer->materialFile->shaderTypeId == 0)
    //                    editor->core->glewContext->drawMesh(renderer->materialFile->programId, renderer->meshFile->VAO,
    //                        renderer->meshFile->indiceCount, VP, editor->sceneCamera->position);
    //                else if (renderer->materialFile->shaderTypeId == 1)
    //                    editor->core->glewContext->drawMesh(renderer, VP, editor->sceneCamera->position);
    //            }
    //        }
    //    }



    //    // draw material spheres here to their offscreenframebuffer
    //    // you can call draw mesh function in renderer
    //    // V: glm::lookat(0), P : standard camPos: find ideal one M: I

    //    editor->core->glewContext->bindFrameBuffer(0);
    //}



}