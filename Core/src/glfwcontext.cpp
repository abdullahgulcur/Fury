#include "pch.h"
#include "glfwcontext.h"
#include "core.h"
#include "lodepng/lodepng.h"

namespace Fury {

	bool GlfwContext::dragAndDropFromOutside;
	std::vector<std::string> GlfwContext::dragAndDropFiles;
	float GlfwContext::scrollOffset;

	GlfwContext::GlfwContext() {

		if (!glfwInit())
			fprintf(stderr, "Failed to initialize GLFW\n");

		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		monitor = glfwGetPrimaryMonitor();
		mode = glfwGetVideoMode(monitor);

		//GLFW_window = glfwCreateWindow(mode->width, mode->height, "Fury", monitor, NULL);

#ifdef EDITOR_MODE
		GLFW_window = glfwCreateWindow(mode->width, mode->height, "Fury", NULL, NULL);
#else
		GLFW_window = glfwCreateWindow(mode->width, mode->height, "Fury", NULL, NULL); // windowed
		//GLFW_window = glfwCreateWindow(mode->width, mode->height, "Fury", monitor, NULL); // fullscreen
#endif // EDITOR_MODE


		glfwMaximizeWindow(GLFW_window);
		glfwMakeContextCurrent(GLFW_window);

		glfwSetInputMode(GLFW_window, GLFW_STICKY_KEYS, GL_TRUE);
		glfwSetInputMode(GLFW_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		glfwSetDropCallback(GLFW_window, drop_callback);

		GlfwContext::loadTitleBarIcon();

		glfwSetScrollCallback(GLFW_window, GlfwContext::scroll_callback);
		glfwSetKeyCallback(GLFW_window, Core::instance->input->key_callback);
	}

	void GlfwContext::loadTitleBarIcon() {

		GLFWimage image;
		unsigned width;
		unsigned height;

		std::vector<unsigned char> img;
		unsigned error = lodepng::decode(img, width, height, "C:/Projects/Fury/Editor/resource/icons/material.png");

		image.pixels = &img[0];
		image.width = width;
		image.height = height;
		glfwSetWindowIcon(GLFW_window, 1, &image);
	}

	void GlfwContext::update() {

		GlfwContext::handleCallBacks();
		//on end...
		//GlfwContext::resetVerticalScrollOffset();
	}

	void GlfwContext::end() {

		glfwSwapBuffers(GLFW_window);
		glfwPollEvents();
	}

	void GlfwContext::handleCallBacks() {

		if (dragAndDropFromOutside) {

			Core::instance->fileSystem->importFiles(dragAndDropFiles);

			dragAndDropFiles.clear();
			dragAndDropFromOutside = false;
		}
	}

	bool GlfwContext::getOpen() {

		bool open = glfwGetKey(GLFW_window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(GLFW_window) == 0;

		if (!open) {

			//core->saveLoadSystem->saveSceneCamera();
		}

		return open;
	}

	void GlfwContext::terminateGLFW() {

		glfwTerminate();
	}

	void GlfwContext::setTitle(const char* title) {

		glfwSetWindowTitle(GLFW_window, title);
	}

	void GlfwContext::setCursorPos(float x, float y) {

		glfwSetCursorPos(GLFW_window, x, y);
	}

	float GlfwContext::verticalScrollOffset() {

		return scrollOffset;
	}

	void GlfwContext::resetVerticalScrollOffset() {

		scrollOffset = 0;
	}

	void GlfwContext::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		scrollOffset = yoffset;
	}

	void GlfwContext::drop_callback(GLFWwindow* window, int count, const char** paths)
	{
		for (unsigned int i = 0; i < count; ++i)
			dragAndDropFiles.push_back(std::string(paths[i]));

		dragAndDropFromOutside = true;
	}
}