#pragma once

#include "glm/glm.hpp"
#include "meshfile.h"
#include "component/meshrenderer.h"

typedef unsigned int GLuint;

namespace Fury {

	class __declspec(dllexport) GlewContext {

	private:

	public:

		GlewContext();
		unsigned int loadShaders(const char* vertex_file_path, const char* fragment_file_path);
		unsigned int loadPBRShaders(unsigned int programId, const char* vertex_file_path, const char* fragment_file_path,
			std::vector<unsigned int>& activeTextures);
		void initLineVAO(GLuint& cubeVAO, std::vector<glm::vec3>& points);
		void drawLineVAO(GLuint lineRendererProgramID, GLuint cubeVAO, const glm::mat4& MVP);
		void clearScreen(glm::vec3 color);
		void viewport(float width, float height);
		void bindFrameBuffer(GLuint buffer);
		void generateTexture(unsigned int& textureId, unsigned width, unsigned height, unsigned char* image);
		void createFrameBuffer(unsigned int& FBO, unsigned int& RBO, unsigned int& textureBuffer, int sizeX, int sizeY);
		void recreateFrameBuffer(unsigned int& FBO, unsigned int& textureBuffer, unsigned int& RBO, int sizeX, int sizeY);
		void initVAO(unsigned int& VAO, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
		void drawMesh(GLuint programID, GLuint VAO, unsigned int indiceCount, const glm::mat4& VP, glm::vec3& camPos);
		void drawMesh(MeshRenderer* component, const glm::mat4& VP, glm::mat4& model, glm::vec3& camPos);

		inline void genVertexArrays(unsigned int size, unsigned int* arrays);
		inline void genBuffers(unsigned int size, unsigned int* buffers);
		inline void bindVertexArray(unsigned int VAO);
		inline void bindArrayBuffer(unsigned int VBO);
		inline void bindElementArrayBuffer(unsigned int EBO);
		inline void arrayBufferData(std::vector<float>& data);
		inline void elementArrayBufferData(std::vector<unsigned int>& indices);
		inline void enableVertexAttribArray(unsigned int index);
		inline void vertexAttribPointer(unsigned int index, unsigned int size, unsigned int str, void* pointer);
		inline void useProgram(unsigned int programId);
		inline void setVec3(unsigned int programId, std::string uniformName, glm::vec3 value);
		inline void setMat4(unsigned int programId, std::string uniformName, glm::mat4 value);
		inline void drawElements_tri(int indiceCount);
		inline void drawElements_triStrip(int indiceCount);
		inline void setInt(unsigned int programId, std::string uniformName, unsigned int index);
		inline void activeTex(unsigned int index);
		inline void bindTex(unsigned int texId);
		inline void drawArrays(unsigned int mode, unsigned int first, unsigned int indiceCount);
		inline void bindBuffer(unsigned int mode, unsigned int buffer);
		inline void bufferData(unsigned int target, unsigned int size, void* data, unsigned int usage);
		inline void vertexAttribPointer(unsigned int index, unsigned int size, unsigned int type, unsigned int normalized, unsigned int stride, void* pointer);
		inline void drawElements(unsigned int mode, unsigned int count, unsigned int type, void* indices);
		inline void lineWidth(float width);
		inline void deleteRenderBuffers(unsigned int size, unsigned int* renderBuffers);
		inline void deleteFrameBuffers(unsigned int size, unsigned int* frameBuffers);
		inline void deleteTextures(unsigned int size, unsigned int* textures);
		inline void genTextures(unsigned int size, unsigned int* textures);
		inline void bindTexture(unsigned int target, unsigned int texture);
		inline void texParameteri(unsigned int target, unsigned int pname, unsigned int param);
		inline void texImage2D(unsigned int target, unsigned int level, unsigned int internalFormat, unsigned int width,
			unsigned int height, unsigned int border, unsigned int format, unsigned int type, void* data);
		inline void generateMipmap(unsigned int target);
		inline void uniformMatrix4fv(unsigned int location, unsigned int count, unsigned int transpose, float* value);
		inline void uniform1i(unsigned int location, unsigned int v0);
		inline void uniform3fv(unsigned int location, unsigned int count, float* value);
		inline void uniform1f(unsigned int location, float v0);
		inline unsigned int getUniformLocation(unsigned int program, const char* name);
		inline void activeTexture(unsigned int texture);
		inline void deleteVertexArrays(unsigned int n, unsigned int* arrays);
		inline void bindFrameBuffer(unsigned int target, unsigned int framebuffer);
		inline void viewport(int x, int y, int width, int height);
		inline void clearColor(float red, float green, float blue, float alpha);
		inline void clear(unsigned int mask);
		inline void uniform4f(unsigned int location, float v0, float v1, float v2, float v3);
		inline void flush();
		inline void finish();
		inline void pixelStorei(unsigned int pname, int param);
		inline void readPixels(int x, int y, int width, int height, unsigned int format, unsigned int type, void* data);
		inline void genFramebuffers(unsigned int n, unsigned int* ids);
		inline void bindFramebuffer(unsigned int target, unsigned int framebuffer);
		inline void framebufferTexture2D(unsigned int target, unsigned int attachment, unsigned int textarget, unsigned int texture, int level);
		inline void genRenderbuffers(unsigned int n, unsigned int* renderbuffers);
		inline void bindRenderbuffer(unsigned int target, unsigned int renderbuffer);
		inline void renderbufferStorage(unsigned int target, unsigned int internalformat, unsigned int width, unsigned int height);
		inline void framebufferRenderbuffer(unsigned int target, unsigned int attachment, unsigned int renderbufferarget, unsigned int renderbuffer);
		inline unsigned int checkFramebufferStatus(unsigned int target);
		inline void deleteProgram(unsigned int program);

	};
}