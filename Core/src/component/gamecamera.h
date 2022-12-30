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

#pragma once

#include "gamecamera.h"
#include "component/component.h"
#include "glm/glm.hpp"

namespace Fury {

	class Transform;

	class __declspec(dllexport) GameCamera : public Component {

	private:

	public:

		float nearClip = 0.1;
		float farClip = 100000;
		int projectionType = 0;
		int fovAxis = 0;
		float fov = 60.f;
		float aspectRatio = 1.77f;
		int width;
		int height;

		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;
		glm::mat4 projectionViewMatrix;
		glm::vec3 position;

		glm::vec4 planes[6];

		unsigned int textureBuffer;
		unsigned int FBO;
		unsigned int RBO;

		/*
		* Editor gizmo
		*/
		unsigned int gizmoVAO;
		unsigned int gizmoShaderProgramID;

		GameCamera();
		~GameCamera();
		void setResolution(int sizeX, int sizeY);
		void resetResolution(int sizeX, int sizeY);
		void init(Transform* transform, int sizeX, int sizeY);
		void setMatrices(Transform* transform);
		void createFBO();
		void recreateFBO();
		void deleteBuffers();
		void updateProjectionMatrix();
		void updateViewMatrix(Transform* transform);
		float getVerticalFOV();
		void frustum(glm::mat4& view_projection);
		bool intersectsAABB(glm::vec3 start, glm::vec3 end);
		void changeFovAxis(int oldFovAxis);
		float convertFOVVerticalToHorizontal(float fovY, float aspectRatio);
		float convertFOVHorizontalToVertical(float fovX, float aspectRatio);

//#ifdef EDITOR_MODE
		void createEditorGizmos();
		void updateEditorGizmos();
		void createFrustumVAO();
		void drawEditorGizmos(glm::mat4 PV, glm::mat4 model);
//#endif

	};
}