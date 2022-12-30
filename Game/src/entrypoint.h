#pragma once

#include "core.h"
#include "entity.h"

namespace Fury {

	class Core;
	class Entity;
}

using namespace Fury;

namespace Game {

	class Game;
	Game* game = NULL;

	extern "C" __declspec(dllexport) void start(UINT* param);

	extern "C" __declspec(dllexport) void update(UINT* param);

	class __declspec(dllexport) Game {

	private:

		Core* core;

	public:

		Game(Core* core);
		~Game();
		void start();
		void update();
		void destroy();
	};
}