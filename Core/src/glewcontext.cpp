#include "pch.h"
#include "glewcontext.h"

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

namespace Fury {

	GlewContext::GlewContext() {

		glewExperimental = true;

		if (glewInit() != GLEW_OK)
			fprintf(stderr, "Failed to initialize GLEW\n");

		//glFrontFace(GL_CCW); // change this to ccw, its default value
		glCullFace(GL_BACK);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glLineWidth(0.5f);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_CULL_FACE);
		// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
		//glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}

	unsigned int GlewContext::loadShaders(const char* vertex_file_path, const char* fragment_file_path) {

		//Create the shaders
		unsigned int VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
		unsigned int FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

		// Read the Vertex Shader code from the file
		std::string VertexShaderCode;
		std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
		if (VertexShaderStream.is_open()) {
			std::stringstream sstr;
			sstr << VertexShaderStream.rdbuf();
			VertexShaderCode = sstr.str();
			VertexShaderStream.close();
		}
		else {
			printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
			getchar();
			return 0;
		}

		// Read the Fragment Shader code from the file
		std::string FragmentShaderCode;
		std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
		if (FragmentShaderStream.is_open()) {
			std::stringstream sstr;
			sstr << FragmentShaderStream.rdbuf();
			FragmentShaderCode = sstr.str();
			FragmentShaderStream.close();
		}

		int Result = GL_FALSE;
		int InfoLogLength;

		// Compile Vertex Shader
		printf("Compiling shader : %s\n", vertex_file_path);
		char const* VertexSourcePointer = VertexShaderCode.c_str();
		glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
		glCompileShader(VertexShaderID);

		// Check Vertex Shader
		glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
			printf("%s\n", &VertexShaderErrorMessage[0]);
		}

		// Compile Fragment Shader
		printf("Compiling shader : %s\n", fragment_file_path);
		char const* FragmentSourcePointer = FragmentShaderCode.c_str();
		glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
		glCompileShader(FragmentShaderID);

		// Check Fragment Shader
		glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
			printf("%s\n", &FragmentShaderErrorMessage[0]);
		}

		// Link the program
		printf("Linking program\n");
		unsigned int ProgramID = glCreateProgram();
		glAttachShader(ProgramID, VertexShaderID);
		glAttachShader(ProgramID, FragmentShaderID);
		glLinkProgram(ProgramID);

		// Check the program
		glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
			glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
			printf("%s\n", &ProgramErrorMessage[0]);
		}

		glDetachShader(ProgramID, VertexShaderID);
		glDetachShader(ProgramID, FragmentShaderID);

		glDeleteShader(VertexShaderID);
		glDeleteShader(FragmentShaderID);

		return ProgramID;
	}

	unsigned int GlewContext::loadPBRShaders(unsigned int pbrShaderProgramId, const char* vertex_file_path, const char* fragment_file_path,
		std::vector<unsigned int>& activeTextures) {

		if(pbrShaderProgramId)
			glDeleteProgram(pbrShaderProgramId);

		//Create the shaders
		unsigned int VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
		unsigned int FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

		// Read the Vertex Shader code from the file
		std::string VertexShaderCode;
		std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
		if (VertexShaderStream.is_open()) {
			std::stringstream sstr;
			sstr << VertexShaderStream.rdbuf();
			VertexShaderCode = sstr.str();
			VertexShaderStream.close();
		}
		else {
			printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
			getchar();
			return 0;
		}

		// Read the Fragment Shader code from the file
		std::string FragmentShaderCode;
		std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
		if (FragmentShaderStream.is_open()) {
			std::stringstream sstr;
			sstr << FragmentShaderStream.rdbuf();
			FragmentShaderCode = sstr.str();
			FragmentShaderStream.close();
		}

		int Result = GL_FALSE;
		int InfoLogLength;

		// Compile Vertex Shader
		printf("Compiling shader : %s\n", vertex_file_path);
		char const* VertexSourcePointer = VertexShaderCode.c_str();
		glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
		glCompileShader(VertexShaderID);

		// Check Vertex Shader
		glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
			printf("%s\n", &VertexShaderErrorMessage[0]);
		}

		char const* fragmentShaderArr[2];
		std::string macro = "#version 330 core\n";

		for (int i = 0; i < activeTextures.size(); i++)
			macro += "#define TEXTURE" + std::to_string(activeTextures[i]) + "\n";

		char const* FragmentSourcePointer = FragmentShaderCode.c_str();
		fragmentShaderArr[0] = macro.c_str();
		fragmentShaderArr[1] = FragmentSourcePointer;

		// Compile Fragment Shader
		printf("Compiling shader : %s\n", fragment_file_path);
		glShaderSource(FragmentShaderID, 2, fragmentShaderArr, NULL);
		glCompileShader(FragmentShaderID);

		//// Compile Fragment Shader
		//printf("Compiling shader : %s\n", fragment_file_path);
		//char const* FragmentSourcePointer = FragmentShaderCode.c_str();
		//glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
		//glCompileShader(FragmentShaderID);

		// Check Fragment Shader
		glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
			printf("%s\n", &FragmentShaderErrorMessage[0]);
		}

		// Link the program
		printf("Linking program\n");
		unsigned int ProgramID = glCreateProgram();
		glAttachShader(ProgramID, VertexShaderID);
		glAttachShader(ProgramID, FragmentShaderID);
		glLinkProgram(ProgramID);

		// Check the program
		glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
			glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
			printf("%s\n", &ProgramErrorMessage[0]);
		}

		glDetachShader(ProgramID, VertexShaderID);
		glDetachShader(ProgramID, FragmentShaderID);

		glDeleteShader(VertexShaderID);
		glDeleteShader(FragmentShaderID);

		return ProgramID;
	}

	unsigned int GlewContext::loadPBRShaders(const char* vertex_file_path, const char* fragment_file_path) {
		//Create the shaders
		unsigned int VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
		unsigned int FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

		// Read the Vertex Shader code from the file
		std::string VertexShaderCode;
		std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
		if (VertexShaderStream.is_open()) {
			std::stringstream sstr;
			sstr << VertexShaderStream.rdbuf();
			VertexShaderCode = sstr.str();
			VertexShaderStream.close();
		}
		else {
			printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
			getchar();
			return 0;
		}

		// Read the Fragment Shader code from the file
		std::string FragmentShaderCode;
		std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
		if (FragmentShaderStream.is_open()) {
			std::stringstream sstr;
			sstr << FragmentShaderStream.rdbuf();
			FragmentShaderCode = sstr.str();
			FragmentShaderStream.close();
		}

		int Result = GL_FALSE;
		int InfoLogLength;

		// Compile Vertex Shader
		printf("Compiling shader : %s\n", vertex_file_path);
		char const* VertexSourcePointer = VertexShaderCode.c_str();
		glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
		glCompileShader(VertexShaderID);

		// Check Vertex Shader
		glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
			printf("%s\n", &VertexShaderErrorMessage[0]);
		}

		// Compile Fragment Shader
		printf("Compiling shader : %s\n", fragment_file_path);
		char const* FragmentSourcePointer = FragmentShaderCode.c_str();
		glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
		glCompileShader(FragmentShaderID);

		// Check Fragment Shader
		glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
			printf("%s\n", &FragmentShaderErrorMessage[0]);
		}

		// Link the program
		printf("Linking program\n");
		unsigned int ProgramID = glCreateProgram();
		glAttachShader(ProgramID, VertexShaderID);
		glAttachShader(ProgramID, FragmentShaderID);
		glLinkProgram(ProgramID);

		// Check the program
		glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
			glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
			printf("%s\n", &ProgramErrorMessage[0]);
		}

		glDetachShader(ProgramID, VertexShaderID);
		glDetachShader(ProgramID, FragmentShaderID);

		glDeleteShader(VertexShaderID);
		glDeleteShader(FragmentShaderID);

		return ProgramID;
	}

	void GlewContext::initLineVAO(GLuint& cubeVAO, std::vector<glm::vec3>& points) {

		glGenVertexArrays(1, &cubeVAO);
		unsigned int VBO;
		glGenBuffers(1, &VBO);
		glBindVertexArray(cubeVAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(glm::vec3), &points[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void GlewContext::drawLineVAO(GLuint lineRendererProgramID, GLuint cubeVAO, const glm::mat4& MVP) {

		glUseProgram(lineRendererProgramID);
		glUniformMatrix4fv(glGetUniformLocation(lineRendererProgramID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
		glUniform3fv(glGetUniformLocation(lineRendererProgramID, "color"), 1, &glm::vec3(0, 1, 0)[0]);

		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_LINES, 0, 24);
		glBindVertexArray(0);
	}

	void GlewContext::drawMesh(GLuint programID, GLuint VAO, unsigned int indiceCount, const glm::mat4& VP, glm::vec3& camPos) {

		glUseProgram(programID);
		glm::mat4 model(1);
		glUniform3fv(glGetUniformLocation(programID, "camPos"), 1, &camPos[0]);
		glUniformMatrix4fv(glGetUniformLocation(programID, "PV"), 1, GL_FALSE, &VP[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &model[0][0]);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indiceCount, GL_UNSIGNED_INT, (void*)0);
		glBindVertexArray(0);
	}

	//void GlewContext::drawMesh(MeshRenderer* component, const glm::mat4& VP, glm::mat4& model, glm::vec3& camPos) {

	//	// (3) iterate through active texture indices and bind with texture index at shader...
	//	unsigned int programId = component->materialFile->programId;
	//	glUseProgram(programId);
	//	//glm::mat4 model(1);
	//	glUniform3fv(glGetUniformLocation(programId, "camPos"), 1, &camPos[0]);
	//	glUniformMatrix4fv(glGetUniformLocation(programId, "PV"), 1, GL_FALSE, &VP[0][0]);
	//	glUniformMatrix4fv(glGetUniformLocation(programId, "model"), 1, GL_FALSE, &model[0][0]);

	//	for (int i = 0; i < component->materialFile->activeTextureIndices.size(); i++) {

	//		std::string texStr = "texture" + std::to_string(component->materialFile->activeTextureIndices[i]);
	//		glActiveTexture(GL_TEXTURE0 + i);
	//		glBindTexture(GL_TEXTURE_2D, component->materialFile->textureFiles[i]->textureId);
	//		glUniform1i(glGetUniformLocation(programId, &texStr[0]), i);
	//	}

	//	glBindVertexArray(component->meshFile->VAO);
	//	glDrawElements(GL_TRIANGLES, component->meshFile->indiceCount, GL_UNSIGNED_INT, (void*)0);
	//	glBindVertexArray(0);
	//}

	void GlewContext::clearScreen(glm::vec3 color) {

		//glClearColor(0.4f, 0.65f, 0.8f, 0.0f);
		glClearColor(color.r, color.g, color.b, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void  GlewContext::viewport(float width, float height) {

		glViewport(0, 0, width, height);
	}

	void GlewContext::createFrameBuffer(unsigned int& FBO, unsigned int& RBO, unsigned int& textureBuffer, int sizeX, int sizeY) {

		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		glGenTextures(1, &textureBuffer);
		glBindTexture(GL_TEXTURE_2D, textureBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sizeX, sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureBuffer, 0);

		//unsigned int rbo;
		glGenRenderbuffers(1, &RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, sizeX, sizeY);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void GlewContext::recreateFrameBuffer(unsigned int& FBO, unsigned int& textureBuffer, unsigned int& RBO, int sizeX, int sizeY) {

		glDeleteRenderbuffers(1, &RBO);
		glDeleteTextures(1, &textureBuffer);
		glDeleteFramebuffers(1, &FBO);

		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		glGenTextures(1, &textureBuffer);
		glBindTexture(GL_TEXTURE_2D, textureBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sizeX, sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureBuffer, 0);

		glGenRenderbuffers(1, &RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, sizeX, sizeY);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void GlewContext::bindFrameBuffer(GLuint buffer) {
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, buffer));
	}

	void GlewContext::generateTexture(unsigned int& textureId, unsigned width, unsigned height, unsigned char* image) {

		float maxAniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	void GlewContext::initVAO(unsigned int& VAO, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {

		glGenVertexArrays(1, &VAO);

		unsigned int VBO;
		glGenBuffers(1, &VBO);
		unsigned int EBO;
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

		glBindVertexArray(0);
	}

	void GlewContext::genVertexArrays(unsigned int size, unsigned int* arrays) {
		glGenVertexArrays(size, arrays);
	}

	void GlewContext::genBuffers(unsigned int size, unsigned int* buffers) {
		glGenBuffers(size, buffers);
	}

	void GlewContext::bindVertexArray(unsigned int VAO) {
		glBindVertexArray(VAO);
	}

	void GlewContext::bindArrayBuffer(unsigned int VBO) {
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
	}

	void GlewContext::bindElementArrayBuffer(unsigned int EBO) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	}

	void GlewContext::arrayBufferData(std::vector<float>& data) {
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
	}

	void GlewContext::elementArrayBufferData(std::vector<unsigned int>& indices) {
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	}

	void GlewContext::enableVertexAttribArray(unsigned int index) {
		GLCall(glEnableVertexAttribArray(index));
	}

	void GlewContext::vertexAttribPointer(unsigned int index, unsigned int size, unsigned int stride, void* pointer) {
		glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride, pointer);
	}

	void GlewContext::useProgram(unsigned int pbrShaderProgramId) {
		GLCall(glUseProgram(pbrShaderProgramId));
	}

	void GlewContext::setVec3(unsigned int pbrShaderProgramId, std::string uniformName, glm::vec3 value) {
		glUniform3fv(glGetUniformLocation(pbrShaderProgramId, uniformName.c_str()), 1, &value[0]);
	}

	void GlewContext::setMat4(unsigned int pbrShaderProgramId, std::string uniformName, glm::mat4 value) {
		glUniformMatrix4fv(glGetUniformLocation(pbrShaderProgramId, uniformName.c_str()), 1, GL_FALSE, &value[0][0]);
	}

	void GlewContext::drawElements_tri(int indiceCount) {
		glDrawElements(GL_TRIANGLES, indiceCount, GL_UNSIGNED_INT, (void*)0);
	}

	void GlewContext::drawElements_triStrip(int indiceCount) {
		glDrawElements(GL_TRIANGLE_STRIP, indiceCount, GL_UNSIGNED_INT, (void*)0);
	}

	void GlewContext::setInt(unsigned int pbrShaderProgramId, std::string uniformName, unsigned int index) {
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, &uniformName[0]), index);
	}

	void GlewContext::activeTex(unsigned int index) {
		glActiveTexture(GL_TEXTURE0 + index);
	}

	void GlewContext::bindTex(unsigned int texId) {
		glBindTexture(GL_TEXTURE_2D, texId);
	}

	void GlewContext::drawArrays(unsigned int mode, unsigned int first, unsigned int indiceCount) {
		glDrawArrays(mode, first, indiceCount);
	}

	void GlewContext::bindBuffer(unsigned int mode, unsigned int buffer) {
		glBindBuffer(mode, buffer);
	}

	void GlewContext::bufferData(unsigned int target, unsigned int size, void* data, unsigned int usage) {
		glBufferData(target, size, data, usage);
	}
	void GlewContext::vertexAttribPointer(unsigned int index, unsigned int size, unsigned int type, unsigned int normalized, unsigned int stride, void* pointer) {
		GLCall(glVertexAttribPointer(index, size, type, normalized, stride, pointer));
	}

	void GlewContext::drawElements(unsigned int mode, unsigned int count, unsigned int type, void* indices) {
		GLCall(glDrawElements(mode, count, type, indices));
	}

	void GlewContext::lineWidth(float width) {
		glLineWidth(width);
	}

	void GlewContext::deleteRenderBuffers(unsigned int size, unsigned int* renderBuffers) {
		glDeleteRenderbuffers(1, renderBuffers);
	}

	void GlewContext::deleteFrameBuffers(unsigned int size, unsigned int* frameBuffers) {
		glDeleteFramebuffers(1, frameBuffers);
	}

	void GlewContext::deleteTextures(unsigned int size, unsigned int* textures) {
		glDeleteTextures(1, textures);
	}

	void GlewContext::genTextures(unsigned int size, unsigned int* textures) {
		GLCall(glGenTextures(size, textures));
	}

	void GlewContext::bindTexture(unsigned int target, unsigned int texture) {
		GLCall(glBindTexture(target, texture));
	}

	void GlewContext::texParameteri(unsigned int target, unsigned int pname, unsigned int param) {
		GLCall(glTexParameteri(target, pname, param));
	}

	void GlewContext::texImage2D(unsigned int target, unsigned int level, unsigned int internalFormat, unsigned int width,
		unsigned int height, unsigned int border, unsigned int format, unsigned int type, void* data) {
		GLCall(glTexImage2D(target, level, internalFormat, width, height, border, format, type, data));
	}

	void GlewContext::generateMipmap(unsigned int target) {
		GLCall(glGenerateMipmap(target));
	}

	void GlewContext::uniformMatrix4fv(unsigned int location, unsigned int count, unsigned int transpose, float* value) {
		GLCall(glUniformMatrix4fv(location, count, transpose, value));
	}

	void GlewContext::uniform1i(unsigned int location, unsigned int v0) {
		GLCall(glUniform1i(location, v0));
	}

	void GlewContext::uniform3fv(unsigned int location, unsigned int count, float* value) {
		GLCall(glUniform3fv(location, count, value));
	}

	void GlewContext::uniform2fv(unsigned int location, unsigned int count, float* value) {
		GLCall(glUniform2fv(location, count, value));
	}

	void GlewContext::uniform2f(unsigned int location, float v0, float v1) {
		GLCall(glUniform2f(location, v0, v1));
	}

	void GlewContext::uniform1f(unsigned int location, float v0) {
		GLCall(glUniform1f(location, v0));
	}

	void GlewContext::uniform3f(unsigned int location, float v0, float v1, float v2) {
		GLCall(glUniform3f(location, v0, v1, v2));
	}

	unsigned int GlewContext::getUniformLocation(unsigned int program, const char* name) {
		GLCall(return glGetUniformLocation(program, name));
	}

	void GlewContext::activeTexture(unsigned int texture) {
		GLCall(glActiveTexture(texture));
	}

	void GlewContext::deleteVertexArrays(unsigned int n, unsigned int* arrays) {
		glDeleteVertexArrays(n, arrays);
	}

	void GlewContext::bindFrameBuffer(unsigned int target, unsigned int framebuffer) {
		glBindFramebuffer(target, framebuffer);
	}

	void GlewContext::viewport(int x, int y, int width, int height) {
		glViewport(x, y, width, height);
	}

	void GlewContext::clearColor(float red, float green, float blue, float alpha) {
		glClearColor(red, green, blue, alpha);
	}

	void GlewContext::clear(unsigned int mask) {
		glClear(mask);
	}

	void GlewContext::uniform4f(unsigned int location, float v0, float v1, float v2, float v3) {
		glUniform4f(location, v0, v1, v2, v3);
	}

	void GlewContext::flush() {
		glFlush();
	}

	void GlewContext::finish() {
		glFinish();
	}

	void GlewContext::pixelStorei(unsigned int pname, int param) {
		glPixelStorei(pname, param);
	}

	void GlewContext::readPixels(int x, int y, int width, int height, unsigned int format, unsigned int type, void* data) {
		glReadPixels(x, y, width, height, format, type, data);
	}

	void GlewContext::genFramebuffers(unsigned int n, unsigned int* ids) {
		glGenFramebuffers(1, ids);
	}

	void GlewContext::bindFramebuffer(unsigned int target, unsigned int framebuffer) {
		glBindFramebuffer(target, framebuffer);
	}

	void GlewContext::framebufferTexture2D(unsigned int target, unsigned int attachment, unsigned int textarget, unsigned int texture, int level) {
		glFramebufferTexture2D(target, attachment, textarget, texture, level);
	}

	void GlewContext::genRenderbuffers(unsigned int n, unsigned int* renderbuffers) {
		glGenRenderbuffers(n, renderbuffers);
	}

	void GlewContext::bindRenderbuffer(unsigned int target, unsigned int renderbuffer) {
		glBindRenderbuffer(target, renderbuffer);
	}

	void GlewContext::renderbufferStorage(unsigned int target, unsigned int internalformat, unsigned int width, unsigned int height) {
		glRenderbufferStorage(target, internalformat, width, height);
	}

	void GlewContext::framebufferRenderbuffer(unsigned int target, unsigned int attachment, unsigned int renderbufferarget, unsigned int renderbuffer) {
		glFramebufferRenderbuffer(target, attachment, renderbufferarget, renderbuffer);
	}

	unsigned int GlewContext::checkFramebufferStatus(unsigned int target) {
		return glCheckFramebufferStatus(target);
	}

	void GlewContext::deleteProgram(unsigned int program) {
		glDeleteProgram(program);
	}

	void GlewContext::polygonMode(unsigned int face, unsigned int mode) {
		glPolygonMode(face, mode);
	}

	void GlewContext::texStorage3D(unsigned int target, unsigned int levels, unsigned int internalFormat, unsigned int width, unsigned int height, unsigned int depth) {
		GLCall(glTexStorage3D(target, levels, internalFormat, width, height, depth));
	}

	void GlewContext::texSubImage3D(unsigned int target, unsigned int level, int xoffset, int yoffset, int zoffset, unsigned int width, unsigned int height, unsigned int depth, unsigned int format, unsigned int type, const void* pixels) {
		GLCall(glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels));
	}

	void GlewContext::vertexAttribDivisor(unsigned int index, unsigned int divisor) {
		GLCall(glVertexAttribDivisor(index, divisor));
	}

	void GlewContext::drawElementsInstanced(unsigned int mode, unsigned int count, unsigned int type, const void* indices, unsigned int primcount) {
		GLCall(glDrawElementsInstanced(mode, count, type, indices, primcount));
	}

	void GlewContext::deleteBuffers(unsigned int size, const unsigned int* buffers) {
		GLCall(glDeleteBuffers(size, buffers));
	}

	void GlewContext::enable(unsigned int cap) {
		glEnable(cap);
	}

	void GlewContext::depthFunc(unsigned int func) {
		glDepthFunc(func);
	}
}