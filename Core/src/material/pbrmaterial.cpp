#include "pch.h"
#include "pbrmaterial.h"
#include "glewcontext.h"
#include "core.h"
#include "FreeImage.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

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

	PBRMaterial::PBRMaterial() {

		GlewContext* glew = Core::instance->glewContext;
		pbrShaderProgramId_old = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/PBR.vert", "C:/Projects/Fury/Core/src/shader/PBR.frag");

		pbrShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/pbr/pbr.vert", "C:/Projects/Fury/Core/src/shader/pbr/pbr.frag");
		equirectangularToCubemapShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/pbr/cubemap.vert", "C:/Projects/Fury/Core/src/shader/pbr/equirectangular_to_cubemap.frag");
		irradianceShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/pbr/cubemap.vert", "C:/Projects/Fury/Core/src/shader/pbr/irradiance_convolution.frag");
		prefilterShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/pbr/cubemap.vert", "C:/Projects/Fury/Core/src/shader/pbr/prefilter.frag");
		brdfShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/pbr/brdf.vert", "C:/Projects/Fury/Core/src/shader/pbr/brdf.frag");
		backgroundShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/pbr/background.vert", "C:/Projects/Fury/Core/src/shader/pbr/background.frag");

		PBRMaterial::createCubeVAO();
		PBRMaterial::createQuadVAO();

		glUseProgram(pbrShaderProgramId);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "irradianceMap"), 0);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "prefilterMap"), 1);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "brdfLUT"), 2);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture0"), 3);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture1"), 4);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture2"), 5);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture3"), 6);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "texture4"), 7);

		glUseProgram(backgroundShaderProgramId);
		glUniform1i(glGetUniformLocation(backgroundShaderProgramId, "environmentMap"), 0);

		lightPositions[0] = glm::vec3(-10.0f, 10.0f, 10.0f);
		lightPositions[1] = glm::vec3(10.0f, 10.0f, 10.0f);
		lightPositions[2] = glm::vec3(-10.0f, -10.0f, 10.0f);
		lightPositions[3] = glm::vec3(10.0f, -10.0f, 10.0f);

		lightColors[0] = glm::vec3(300.f, 300.f, 300.f);
		lightColors[1] = glm::vec3(300.f, 300.f, 300.f);
		lightColors[2] = glm::vec3(300.f, 300.f, 300.f);
		lightColors[3] = glm::vec3(300.f, 300.f, 300.f);

		// pbr: setup framebuffer
		// ----------------------
		unsigned int captureFBO;
		unsigned int captureRBO;
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

		// pbr: load the HDR environment map
		// ---------------------------------

		FREE_IMAGE_FORMAT format = FreeImage_GetFileType("C:/Projects/Fury/Core/src/shader/pbr/sky.hdr", 0);
		FIBITMAP* image = FreeImage_Load(format, "C:/Projects/Fury/Core/src/shader/pbr/sky.hdr");
		FIBITMAP* imageRGB16bits = FreeImage_ConvertToRGBF(image);
		unsigned width = FreeImage_GetWidth(imageRGB16bits);
		unsigned height = FreeImage_GetHeight(imageRGB16bits);
		float* data = (float*)FreeImage_GetBits(imageRGB16bits);

		//// pbr: load the HDR environment map
		//// ---------------------------------
		//stbi_set_flip_vertically_on_load(true);
		//int width, height, nrComponents;
		//float* data = stbi_loadf("C:/Projects/Fury/Core/src/shader/pbr/sky.exr", &width, &height, &nrComponents, 0);
		//unsigned int hdrTexture;
		//if (data)
		//{
		//	glGenTextures(1, &hdrTexture);
		//	glBindTexture(GL_TEXTURE_2D, hdrTexture);
		//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//	stbi_image_free(data);
		//}
		//else
		//{
		//	std::cout << "Failed to load HDR image." << std::endl;
		//}

		unsigned int hdrTexture;
		glGenTextures(1, &hdrTexture);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		FreeImage_Unload(image);
		FreeImage_Unload(imageRGB16bits);

		// pbr: setup cubemap to render to and attach to framebuffer
		// ---------------------------------------------------------
		glGenTextures(1, &envCubemap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
		// ----------------------------------------------------------------------------------------------
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] =
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		// pbr: convert HDR equirectangular environment map to cubemap equivalent
		// ----------------------------------------------------------------------
		glUseProgram(equirectangularToCubemapShaderProgramId);
		glUniform1i(glGetUniformLocation(equirectangularToCubemapShaderProgramId, "equirectangularMap"), 0);
		glUniformMatrix4fv(glGetUniformLocation(equirectangularToCubemapShaderProgramId, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);

		glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glUniformMatrix4fv(glGetUniformLocation(equirectangularToCubemapShaderProgramId, "view"), 1, GL_FALSE, &captureViews[i][0][0]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			PBRMaterial::renderCube();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
		// --------------------------------------------------------------------------------
		glGenTextures(1, &irradianceMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

		// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
		// -----------------------------------------------------------------------------
		glUseProgram(irradianceShaderProgramId);
		glUniform1i(glGetUniformLocation(irradianceShaderProgramId, "environmentMap"), 0);
		glUniformMatrix4fv(glGetUniformLocation(irradianceShaderProgramId, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

		glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glUniformMatrix4fv(glGetUniformLocation(irradianceShaderProgramId, "view"), 1, GL_FALSE, &captureViews[i][0][0]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			PBRMaterial::renderCube();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
		// --------------------------------------------------------------------------------
		glGenTextures(1, &prefilterMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
		// ----------------------------------------------------------------------------------------------------
		glUseProgram(prefilterShaderProgramId);
		glUniform1i(glGetUniformLocation(prefilterShaderProgramId, "environmentMap"), 0);
		glUniformMatrix4fv(glGetUniformLocation(prefilterShaderProgramId, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		unsigned int maxMipLevels = 5;
		for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
		{
			// reisze framebuffer according to mip-level size.
			unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
			unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
			glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
			glViewport(0, 0, mipWidth, mipHeight);

			float roughness = (float)mip / (float)(maxMipLevels - 1);
			glUniform1f(glGetUniformLocation(prefilterShaderProgramId, "roughness"), roughness);

			for (unsigned int i = 0; i < 6; ++i)
			{
				glUniformMatrix4fv(glGetUniformLocation(prefilterShaderProgramId, "view"), 1, GL_FALSE, &captureViews[i][0][0]);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				PBRMaterial::renderCube();
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// pbr: generate a 2D LUT from the BRDF equations used.
		// ----------------------------------------------------
		glGenTextures(1, &brdfLUTTexture);

		// pre-allocate enough memory for the LUT texture.
		glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
		// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

		glViewport(0, 0, 512, 512);
		glUseProgram(brdfShaderProgramId);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderQuad();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

//--------------------------------


		//GlewContext* glew = Core::instance->glewContext;
		//pbrShaderProgramId_old = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/PBR.vert", "C:/Projects/Fury/Core/src/shader/PBR.frag");

		//pbrShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/temp/2.1.1.pbr.vs", "C:/Projects/Fury/Core/src/shader/temp/2.1.1.pbr.fs");
		//equirectangularToCubemapShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/temp/2.1.1.cubemap.vs", "C:/Projects/Fury/Core/src/shader/temp/2.1.1.equirectangular_to_cubemap.fs");
		//backgroundShaderProgramId = glew->loadPBRShaders("C:/Projects/Fury/Core/src/shader/temp/2.1.1.background.vs", "C:/Projects/Fury/Core/src/shader/temp/2.1.1.background.fs");

		//PBRMaterial::createCubeVAO();
		//PBRMaterial::createQuadVAO();

		//glew->useProgram(pbrShaderProgramId);
		//glew->uniform3f(glew->getUniformLocation(pbrShaderProgramId, "albedo"), 0.5f, 0.0f, 0.0f);
		//glew->uniform1f(glew->getUniformLocation(pbrShaderProgramId, "ao"), 1.0f);
		//
		//glew->useProgram(backgroundShaderProgramId);
		//glew->uniform1i(glew->getUniformLocation(backgroundShaderProgramId, "environmentMap"), 0);

		//lightPositions[0] = glm::vec3(-10.0f, 10.0f, 10.0f);
		//lightPositions[1] = glm::vec3(10.0f, 10.0f, 10.0f);
		//lightPositions[2] = glm::vec3(-10.0f, -10.0f, 10.0f);
		//lightPositions[3] = glm::vec3(10.0f, -10.0f, 10.0f);

		//lightColors[0] = glm::vec3(300.f, 300.f, 300.f);
		//lightColors[1] = glm::vec3(300.f, 300.f, 300.f);
		//lightColors[2] = glm::vec3(300.f, 300.f, 300.f);
		//lightColors[3] = glm::vec3(300.f, 300.f, 300.f);

		//// pbr: setup framebuffer
		//// ----------------------
		//unsigned int captureFBO;
		//unsigned int captureRBO;
		//glew->genFramebuffers(1, &captureFBO);
		//glew->genRenderbuffers(1, &captureRBO);

		//glew->bindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		//glew->bindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		//glew->renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		//glew->framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

		//// pbr: load the HDR environment map
		//// ---------------------------------
		//stbi_set_flip_vertically_on_load(true);
		//int width, height, nrComponents;
		//float* data = stbi_loadf("C:/Projects/Fury/Core/src/shader/pbr/newport_loft.hdr", &width, &height, &nrComponents, 0);
		//unsigned int hdrTexture;
		//if (data)
		//{
		//	glew->genTextures(1, &hdrTexture);
		//	glew->bindTexture(GL_TEXTURE_2D, hdrTexture);
		//	glew->texImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

		//	glew->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//	glew->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//	glew->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//	glew->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//	stbi_image_free(data);
		//}
		//else
		//{
		//	std::cout << "Failed to load HDR image." << std::endl;
		//}

		//// pbr: setup cubemap to render to and attach to framebuffer
		//// ---------------------------------------------------------
		//glew->genTextures(1, &envCubemap);
		//glew->bindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		//for (unsigned int i = 0; i < 6; ++i)
		//{
		//	glew->texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
		//}
		//glew->texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glew->texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//glew->texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		//glew->texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glew->texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
		//// ----------------------------------------------------------------------------------------------
		//glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		//glm::mat4 captureViews[] =
		//{
		//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		//};

		//// pbr: convert HDR equirectangular environment map to cubemap equivalent
		//// ----------------------------------------------------------------------
		//glew->useProgram(equirectangularToCubemapShaderProgramId);
		//glew->uniform1i(glew->getUniformLocation(equirectangularToCubemapShaderProgramId, "equirectangularMap"), 0);
		//glew->uniformMatrix4fv(glew->getUniformLocation(equirectangularToCubemapShaderProgramId, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
		//glew->activeTexture(GL_TEXTURE0);
		//glew->bindTexture(GL_TEXTURE_2D, hdrTexture);

		//glew->viewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
		//glew->bindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		//for (unsigned int i = 0; i < 6; ++i)
		//{
		//	glew->uniformMatrix4fv(glew->getUniformLocation(equirectangularToCubemapShaderProgramId, "view"), 1, GL_FALSE, &captureViews[i][0][0]);
		//	glew->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		//	glew->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//	PBRMaterial::renderCube();
		//}
		//glew->bindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void PBRMaterial::createCubeVAO() {

		GlewContext* glew = Core::instance->glewContext;

		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left                
		};
		glew->genVertexArrays(1, &cubeVAO);
		unsigned int cubeVBO;
		glew->genBuffers(1, &cubeVBO);
		// fill buffer
		glew->bindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glew->bufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glew->bindVertexArray(cubeVAO);
		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glew->enableVertexAttribArray(1);
		glew->vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glew->enableVertexAttribArray(2);
		glew->vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glew->bindBuffer(GL_ARRAY_BUFFER, 0);
		glew->bindVertexArray(0);
	}

	void PBRMaterial::createQuadVAO()
	{
		GlewContext* glew = Core::instance->glewContext;

		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glew->genVertexArrays(1, &quadVAO);
		unsigned int quadVBO;
		glew->genBuffers(1, &quadVBO);
		glew->bindVertexArray(quadVAO);
		glew->bindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glew->bufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glew->enableVertexAttribArray(0);
		glew->vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glew->enableVertexAttribArray(1);
		glew->vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}

	void PBRMaterial::renderCube() {

		GlewContext* glew = Core::instance->glewContext;

		glew->bindVertexArray(cubeVAO);
		glew->drawArrays(GL_TRIANGLES, 0, 36);
		glew->bindVertexArray(0);
	}

	void PBRMaterial::renderQuad() {

		GlewContext* glew = Core::instance->glewContext;

		glew->bindVertexArray(quadVAO);
		glew->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glew->bindVertexArray(0);
	}

	// ..mat()

	// .... mat()
	// butun materialler tek bir dosyada olali

}