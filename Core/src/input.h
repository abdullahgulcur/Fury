#pragma once

#include "include/GLFW/glfw3.h"

namespace Fury {

	enum class __declspec(dllexport) KeyCode { W, A, S, D, space, left_shift, enter, left_ctrl, left_alt, esc, up, down, left, right };

	class __declspec(dllexport) Input {

	private:

		// use bitmask
		static bool w_pressed;
		static bool a_pressed;
		static bool s_pressed;
		static bool d_pressed;
		static bool space_pressed;
		static bool left_shift_pressed;
		static bool enter_pressed;
		static bool left_ctrl_pressed;
		static bool left_alt_pressed;
		static bool esc_pressed;
		static bool up_pressed;
		static bool down_pressed;
		static bool left_pressed;
		static bool right_pressed;

	public:

		Input();
		~Input();
		static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static bool getKeyDown(KeyCode keyCode);
		//static bool getKey(KeyCode keyCode);
		//static bool getKeyUp(KeyCode keyCode);

	};
}