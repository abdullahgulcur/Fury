#pragma once

#include "GL/glew.h"
#include "include/GLFW/glfw3.h"

namespace Fury {

	class Core;

	//__declspec(dllexport) float scrollOffset;
	class __declspec(dllexport) GlfwContext {

	private:

	public:

		GLFWwindow* GLFW_window;
		const GLFWvidmode* mode;
		GLFWmonitor* monitor;
		static float scrollOffset;

		static bool dragAndDropFromOutside;
		static std::vector<std::string> dragAndDropFiles;

		GlfwContext();
		void update(float dt);
		void loadTitleBarIcon();
		void end();
		void handleCallBacks();
		bool getOpen();
		void terminateGLFW();
		void setTitle(const char* title);
		void setCursorPos(float x, float y);
		float verticalScrollOffset();
		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
		void resetVerticalScrollOffset();
		static void drop_callback(GLFWwindow* window, int count, const char** paths);

	};
}