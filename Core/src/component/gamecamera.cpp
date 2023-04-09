/*************************************************************************/
/*  gamecamera.h                                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                               FURY                                    */
/*************************************************************************/
/* Copyright (c) 2021-2022 Abdullah Gulcur                               */
/* Copyright (c) 2021-2022 Fury contributors (cf. AUTHORS.md).           */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "pch.h"
#include "gamecamera.h"
#include "scene.h"
#include "glewcontext.h"
#include "core.h"

namespace Fury {

	GameCamera::GameCamera(Entity* entity) : Component(entity) {}

    GameCamera::~GameCamera() {

#ifdef EDITOR_MODE
        GlewContext* glew = Core::instance->glewContext;
        glew->deleteVertexArrays(1, &gizmoVAO);
        glew->deleteProgram(gizmoShaderProgramID);
#endif  

        GameCamera::deleteBuffers();
        Core::instance->sceneManager->currentScene->primaryCamera = NULL;
    }

    void GameCamera::start() {

    }

    void GameCamera::update(float dt) {

    }

    void GameCamera::init(Transform* transform) { //, int sizeX, int sizeY

        //this->width = sizeX;
        //this->height = sizeY;
        /*this->aspectRatio = (float)sizeX / sizeY;*/
        this->aspectRatio = (float)width/ height;
        GameCamera::setMatrices(transform);
        GameCamera::frustum(projectionViewMatrix);
        GameCamera::createEditorGizmos();
        GameCamera::createFBO();
    }

    void GameCamera::setResolution() { //int sizeX, int sizeY

        //this->width = sizeX;
        //this->height = sizeY;
        this->aspectRatio = (float)width / height;
        GameCamera::updateProjectionMatrix();
    }

    void GameCamera::resetResolution() { //int sizeX, int sizeY

        /*this->width = sizeX;
        this->height = sizeY;*/
        this->aspectRatio = (float)width / height;
        GameCamera::updateProjectionMatrix();
        GameCamera::recreateFBO();
        GameCamera::updateEditorGizmos();
        GameCamera::frustum(projectionViewMatrix);
    }

    void GameCamera::setMatrices(Transform* transform) {

        glm::vec3 direction = transform->model * glm::vec4(0, 0, 1, 0);
        glm::vec3 right = transform->model * glm::vec4(-1, 0, 0, 0);
        glm::vec3 up = glm::cross(right, direction);

        projectionMatrix = glm::perspective(GameCamera::getVerticalFOV(), aspectRatio, nearClip, farClip);
        position = transform->getGlobalPosition();
        viewMatrix = glm::lookAt(position, position + direction, up);
        projectionViewMatrix = projectionMatrix * viewMatrix;
    }

    void GameCamera::updateViewMatrix(Transform* transform) {

        glm::vec3 direction = transform->model * glm::vec4(0, 0, 1, 0);
        glm::vec3 right = transform->model * glm::vec4(-1, 0, 0, 0);
        glm::vec3 up = glm::cross(right, direction);

        position = transform->getGlobalPosition();
        viewMatrix = glm::lookAt(position, position + direction, up);
        projectionViewMatrix = projectionMatrix * viewMatrix;
        GameCamera::frustum(projectionViewMatrix);
    }

    void GameCamera::createFBO() {

        GlewContext* glew = Core::instance->glewContext;
        glew->genFramebuffers(1, &FBO);
        glew->bindFramebuffer(0x8D40, FBO);

        glew->genTextures(1, &textureBuffer);
        glew->bindTexture(0x0DE1, textureBuffer);
        glew->texImage2D(0x0DE1, 0, 0x1907, width, height, 0, 0x1907, 0x1401, NULL);
        glew->texParameteri(0x0DE1, 0x2801, 0x2601);
        glew->texParameteri(0x0DE1, 0x2800, 0x2601);

        glew->framebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, textureBuffer, 0);

        glew->genRenderbuffers(1, &RBO);
        glew->bindRenderbuffer(0x8D41, RBO);
        glew->renderbufferStorage(0x8D41, 0x88F0, width, height);
        glew->bindRenderbuffer(0x8D41, 0);

        glew->framebufferRenderbuffer(0x8D40, 0x821A, 0x8D41, RBO);

        if (glew->checkFramebufferStatus(0x8D40) != 0x8CD5)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glew->bindFramebuffer(0x8D40, 0);
    }

    void GameCamera::recreateFBO() {

        GameCamera::deleteBuffers();
        GameCamera::createFBO();
    }

    void GameCamera::deleteBuffers() {

        GlewContext* glew = Core::instance->glewContext;
        glew->deleteRenderBuffers(1, &RBO);
        glew->deleteTextures(1, &textureBuffer);
        glew->deleteFrameBuffers(1, &FBO);
    }

    void GameCamera::updateProjectionMatrix() {

        projectionMatrix = glm::perspective(GameCamera::getVerticalFOV(), aspectRatio, nearClip, farClip);
        projectionViewMatrix = projectionMatrix * viewMatrix;
        GameCamera::frustum(projectionViewMatrix);
    }

    float GameCamera::getVerticalFOV() {

        return fovAxis == 0 ? glm::radians(fov) : GameCamera::convertFOVHorizontalToVertical(glm::radians(fov), aspectRatio);;
    }

    // ref: https://arm-software.github.io/opengl-es-sdk-for-android/terrain.html
    void GameCamera::frustum(glm::mat4& view_projection)
    {
        // Frustum planes are in world space.
        glm::mat4 inv = glm::inverse(view_projection);
        // Get world-space coordinates for clip-space bounds.
        glm::vec4 lbn = inv * glm::vec4(-1, -1, -1, 1);
        glm::vec4 ltn = inv * glm::vec4(-1, 1, -1, 1);
        glm::vec4 lbf = inv * glm::vec4(-1, -1, 1, 1);
        glm::vec4 rbn = inv * glm::vec4(1, -1, -1, 1);
        glm::vec4 rtn = inv * glm::vec4(1, 1, -1, 1);
        glm::vec4 rbf = inv * glm::vec4(1, -1, 1, 1);
        glm::vec4 rtf = inv * glm::vec4(1, 1, 1, 1);
        // Divide by w.
        glm::vec3 lbn_pos = glm::vec3(lbn / lbn.w);
        glm::vec3 ltn_pos = glm::vec3(ltn / ltn.w);
        glm::vec3 lbf_pos = glm::vec3(lbf / lbf.w);
        glm::vec3 rbn_pos = glm::vec3(rbn / rbn.w);
        glm::vec3 rtn_pos = glm::vec3(rtn / rtn.w);
        glm::vec3 rbf_pos = glm::vec3(rbf / rbf.w);
        glm::vec3 rtf_pos = glm::vec3(rtf / rtf.w);
        // Get plane normals for all sides of frustum.
        glm::vec3 left_normal = glm::normalize(glm::cross(lbf_pos - lbn_pos, ltn_pos - lbn_pos));
        glm::vec3 right_normal = glm::normalize(glm::cross(rtn_pos - rbn_pos, rbf_pos - rbn_pos));
        glm::vec3 top_normal = glm::normalize(glm::cross(ltn_pos - rtn_pos, rtf_pos - rtn_pos));
        glm::vec3 bottom_normal = glm::normalize(glm::cross(rbf_pos - rbn_pos, lbn_pos - rbn_pos));
        glm::vec3 near_normal = glm::normalize(glm::cross(ltn_pos - lbn_pos, rbn_pos - lbn_pos));
        glm::vec3 far_normal = glm::normalize(glm::cross(rtf_pos - rbf_pos, lbf_pos - rbf_pos));
        // Plane equations compactly represent a plane in 3D space.
        // We want a way to compute the distance to the plane while preserving the sign to know which side we're on.
        // Let:
        //    O: an arbitrary point on the plane
        //    N: the normal vector for the plane, pointing in the direction
        //       we want to be "positive".
        //    X: Position we want to check.
        //
        // Distance D to the plane can now be expressed as a simple operation:
        // D = dot((X - O), N) = dot(X, N) - dot(O, N)
        //
        // We can reduce this to one dot product by assuming that X is four-dimensional (4th component = 1.0).
        // The normal can be extended to four dimensions as well:
        // X' = vec4(X, 1.0)
        // N' = vec4(N, -dot(O, N))
        //
        // The expression now reduces to: D = dot(X', N')
        planes[0] = glm::vec4(near_normal, -glm::dot(near_normal, lbn_pos));   // Near
        planes[1] = glm::vec4(far_normal, -glm::dot(far_normal, lbf_pos));    // Far
        planes[2] = glm::vec4(left_normal, -glm::dot(left_normal, lbn_pos));   // Left
        planes[3] = glm::vec4(right_normal, -glm::dot(right_normal, rbn_pos));  // Right
        planes[4] = glm::vec4(top_normal, -glm::dot(top_normal, ltn_pos));    // Top
        planes[5] = glm::vec4(bottom_normal, -glm::dot(bottom_normal, lbn_pos)); // Bottom
    }

    // ref: https://arm-software.github.io/opengl-es-sdk-for-android/terrain.html
    bool GameCamera::intersectsAABB(glm::vec4& start, glm::vec4& end)
    {
        // If all corners of an axis-aligned bounding box are on the "wrong side" (negative distance)
        // of at least one of the frustum planes, we can safely cull the mesh.
        glm::vec4 corners[8];

        corners[0] = glm::vec4(start.x, start.y, start.z, 1);
        corners[1] = glm::vec4(start.x, start.y, end.z, 1);
        corners[2] = glm::vec4(start.x, end.y, start.z, 1);
        corners[3] = glm::vec4(start.x, end.y, end.z, 1);
        corners[4] = glm::vec4(end.x, start.y, start.z, 1);
        corners[5] = glm::vec4(end.x, start.y, end.z, 1);
        corners[6] = glm::vec4(end.x, end.y, start.z, 1);
        corners[7] = glm::vec4(end.x, end.y, end.z, 1);

        //for (unsigned int c = 0; c < 8; c++)
        //{
        //    // Require 4-dimensional coordinates for plane equations.
        //    corners[c] = glm::vec4(aabb[c], 1.0f);
        //}
        for (unsigned int p = 0; p < 6; p++)
        {
            bool inside_plane = false;
            for (unsigned int c = 0; c < 8; c++)
            {
                // If dot product > 0, we're "inside" the frustum plane,
                // otherwise, outside.
                if (glm::dot(corners[c], planes[p]) > 0.0f)
                {
                    inside_plane = true;
                    break;
                }
            }
            if (!inside_plane)
                return false;
        }
        return true;
    }

    void GameCamera::changeFovAxis(int oldFovAxis) {

        if (fovAxis == oldFovAxis)
            return;

        fov = oldFovAxis == 0 ? glm::degrees(GameCamera::convertFOVVerticalToHorizontal(glm::radians(fov), aspectRatio))
            : glm::degrees(GameCamera::convertFOVHorizontalToVertical(glm::radians(fov), aspectRatio));
    }

    float GameCamera::convertFOVVerticalToHorizontal(float fovY, float aspectRatio) {

        return glm::atan(glm::tan(fovY / 2) * aspectRatio) * 2;
    }

    float GameCamera::convertFOVHorizontalToVertical(float fovX, float aspectRatio) {

        return glm::atan(glm::tan(fovX / 2) / aspectRatio) * 2;
    }

    void GameCamera::createEditorGizmos() {

        GlewContext* glew = Core::instance->glewContext;
        gizmoShaderProgramID = Core::instance->glewContext->loadShaders("C:/Projects/Fury/Core/src/shader/Line.vert", "C:/Projects/Fury/Core/src/shader/Line.frag");
        GameCamera::createFrustumVAO();
    }

    void GameCamera::updateEditorGizmos() {

        GlewContext* glew = Core::instance->glewContext;
        glew->deleteVertexArrays(1, &gizmoVAO);

        float horizontalFOV = 0;
        float verticalFOV = 0;

        if (fovAxis == 0) {
            verticalFOV = glm::radians(fov);
            horizontalFOV = GameCamera::convertFOVVerticalToHorizontal(glm::radians(fov), aspectRatio);
        }
        else {
            verticalFOV = GameCamera::convertFOVHorizontalToVertical(glm::radians(fov), aspectRatio);
            horizontalFOV = glm::radians(fov);
        }
        GameCamera::updateProjectionMatrix();
        GameCamera::createFrustumVAO();
    }

    void GameCamera::createFrustumVAO() {

        float horizontalFOV = 0;
        float verticalFOV = 0;

        if (fovAxis == 0) {
            verticalFOV = glm::radians(fov);
            horizontalFOV = GameCamera::convertFOVVerticalToHorizontal(glm::radians(fov), aspectRatio);
        }
        else {
            verticalFOV = GameCamera::convertFOVHorizontalToVertical(glm::radians(fov), aspectRatio);
            horizontalFOV = glm::radians(fov);
        }

        float xn = glm::tan(horizontalFOV / 2) * nearClip;
        float yn = glm::tan(verticalFOV / 2) * nearClip;
        float xf = glm::tan(horizontalFOV / 2) * farClip;
        float yf = glm::tan(verticalFOV / 2) * farClip;

        std::vector<glm::vec3> points;

        points.push_back(glm::vec3(xn, yn, nearClip));
        points.push_back(glm::vec3(-xn, yn, nearClip));
        points.push_back(glm::vec3(-xn, yn, nearClip));
        points.push_back(glm::vec3(-xn, -yn, nearClip));
        points.push_back(glm::vec3(-xn, -yn, nearClip));
        points.push_back(glm::vec3(xn, -yn, nearClip));
        points.push_back(glm::vec3(xn, -yn, nearClip));
        points.push_back(glm::vec3(xn, yn, nearClip));

        points.push_back(glm::vec3(xf, yf, farClip));
        points.push_back(glm::vec3(-xf, yf, farClip));
        points.push_back(glm::vec3(-xf, yf, farClip));
        points.push_back(glm::vec3(-xf, -yf, farClip));
        points.push_back(glm::vec3(-xf, -yf, farClip));
        points.push_back(glm::vec3(xf, -yf, farClip));
        points.push_back(glm::vec3(xf, -yf, farClip));
        points.push_back(glm::vec3(xf, yf, farClip));

        points.push_back(glm::vec3(xn, yn, nearClip));
        points.push_back(glm::vec3(xf, yf, farClip));
        points.push_back(glm::vec3(-xn, yn, nearClip));
        points.push_back(glm::vec3(-xf, yf, farClip));
        points.push_back(glm::vec3(xn, -yn, nearClip));
        points.push_back(glm::vec3(xf, -yf, farClip));
        points.push_back(glm::vec3(-xn, -yn, nearClip));
        points.push_back(glm::vec3(-xf, -yf, farClip));

        GlewContext* glew = Core::instance->glewContext;
        glew->genVertexArrays(1, &gizmoVAO);
        unsigned int VBO;
        glew->genBuffers(1, &VBO);
        glew->bindVertexArray(gizmoVAO);

        glew->bindBuffer(0x8892, VBO);
        glew->bufferData(0x8892, points.size() * sizeof(glm::vec3), &points[0], 0x88E4);

        glew->enableVertexAttribArray(0);
        glew->vertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), (void*)0);

        glew->bindBuffer(0x8892, 0);
        glew->bindVertexArray(0);
    }

    void GameCamera::drawEditorGizmos(glm::mat4 PV, glm::mat4 model) {

        GlewContext* glew = Core::instance->glewContext;

        glew->useProgram(gizmoShaderProgramID);
        glew->uniformMatrix4fv(glew->getUniformLocation(gizmoShaderProgramID, "PV"), 1, 0, &PV[0][0]);
        glew->uniformMatrix4fv(glew->getUniformLocation(gizmoShaderProgramID, "model"), 1, 0, &model[0][0]);
        glew->uniform3fv(glew->getUniformLocation(gizmoShaderProgramID, "color"), 1, &glm::vec3(0.75f, 0.75f, 0.75f)[0]);

        glew->bindVertexArray(gizmoVAO);
        glew->drawArrays(0x0001, 0, 24);
        glew->bindVertexArray(0);
    }

}