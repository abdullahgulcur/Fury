#include "pch.h"
#include "input.h"

namespace Fury {

	bool Input::w_pressed;
	bool Input::a_pressed;
	bool Input::s_pressed;
	bool Input::d_pressed;
	bool Input::space_pressed;
	bool Input::left_shift_pressed;
	bool Input::enter_pressed;
	bool Input::left_ctrl_pressed;
	bool Input::left_alt_pressed;
	bool Input::esc_pressed;
	bool Input::up_pressed;
	bool Input::down_pressed;
	bool Input::left_pressed;
	bool Input::right_pressed;

	Input::Input() {

	}

	Input::~Input() {

	}

	void Input::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

		if (action == GLFW_PRESS) {
			switch (key) {
			case GLFW_KEY_W: Input::w_pressed = true; break;
			case GLFW_KEY_A: Input::a_pressed = true; break;
			case GLFW_KEY_S: Input::s_pressed = true; break;
			case GLFW_KEY_D: Input::d_pressed = true; break;
			case GLFW_KEY_SPACE: Input::space_pressed = true; break;
			case GLFW_KEY_LEFT_SHIFT: Input::left_shift_pressed = true; break;
			case GLFW_KEY_ENTER: Input::enter_pressed = true; break;
			case GLFW_KEY_LEFT_CONTROL: Input::left_ctrl_pressed = true; break;
			case GLFW_KEY_LEFT_ALT: Input::left_alt_pressed = true; break;
			case GLFW_KEY_ESCAPE: Input::esc_pressed = true; break;
			case GLFW_KEY_UP: Input::up_pressed = true; break;
			case GLFW_KEY_DOWN: Input::down_pressed = true; break;
			case GLFW_KEY_LEFT: Input::left_pressed = true; break;
			case GLFW_KEY_RIGHT: Input::right_pressed = true; break;
			default: break;
			}
		}

		if (action == GLFW_RELEASE) {
			switch (key) {
			case GLFW_KEY_W: Input::w_pressed = false; break;
			case GLFW_KEY_A: Input::a_pressed = false; break;
			case GLFW_KEY_S: Input::s_pressed = false; break;
			case GLFW_KEY_D: Input::d_pressed = false; break;
			case GLFW_KEY_SPACE: Input::space_pressed = false; break;
			case GLFW_KEY_LEFT_SHIFT: Input::left_shift_pressed = false; break;
			case GLFW_KEY_ENTER: Input::enter_pressed = false; break;
			case GLFW_KEY_LEFT_CONTROL: Input::left_ctrl_pressed = false; break;
			case GLFW_KEY_LEFT_ALT: Input::left_alt_pressed = false; break;
			case GLFW_KEY_ESCAPE: Input::esc_pressed = false; break;
			case GLFW_KEY_UP: Input::up_pressed = false; break;
			case GLFW_KEY_DOWN: Input::down_pressed = false; break;
			case GLFW_KEY_LEFT: Input::left_pressed = false; break;
			case GLFW_KEY_RIGHT: Input::right_pressed = false; break;
			default: break;
			}
		}
	}

	bool Input::getKeyDown(KeyCode keyCode) {

		switch (keyCode) {
		case KeyCode::W: return Input::w_pressed;
		case KeyCode::A: return Input::a_pressed;
		case KeyCode::S: return Input::s_pressed;
		case KeyCode::D: return Input::d_pressed;
		case KeyCode::space: return Input::space_pressed;
		case KeyCode::left_shift: return Input::left_shift_pressed;
		case KeyCode::enter: return Input::enter_pressed;
		case KeyCode::left_ctrl:return  Input::left_ctrl_pressed;
		case KeyCode::left_alt:return  Input::left_alt_pressed;
		case KeyCode::esc:return  Input::esc_pressed;
		case KeyCode::up: return Input::up_pressed;
		case KeyCode::down:return  Input::down_pressed;
		case KeyCode::left:return  Input::left_pressed;
		case KeyCode::right:return  Input::right_pressed;
		default: break;
		}
	}

	//bool Input::getKeyUp(KeyCode keyCode) {

	//	switch (keyCode) {
	//	case KeyCode::W: return Input::w_pressed;
	//	case KeyCode::A: return Input::a_pressed;
	//	case KeyCode::S: return Input::s_pressed;
	//	case KeyCode::D: return Input::d_pressed;
	//	case KeyCode::space: return Input::space_pressed;
	//	case KeyCode::left_shift: return Input::left_shift_pressed;
	//	case KeyCode::enter: return Input::enter_pressed;
	//	case KeyCode::left_ctrl:return  Input::left_ctrl_pressed;
	//	case KeyCode::left_alt:return  Input::left_alt_pressed;
	//	case KeyCode::esc:return  Input::esc_pressed;
	//	case KeyCode::up: return Input::up_pressed;
	//	case KeyCode::down:return  Input::down_pressed;
	//	case KeyCode::left:return  Input::left_pressed;
	//	case KeyCode::right:return  Input::right_pressed;
	//	default: break;
	//	}
	//}

}