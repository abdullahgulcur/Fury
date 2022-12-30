#include "pch.h"
#include "entrypoint.h"
#include "scene.h"

namespace Game {

	void start(UINT* param) {

		Core* core = (Core*)param;
		game = new Game(core);
	}

	void update(UINT* param) {

		game->update();
	}

	Game::Game(Core* core) {

		this->core = core;
		Game::start();
	}

	Game::~Game() {

	}

	void Game::start() {

		//Entity* root = core->sceneManager->currentScene->root;
	}

	void Game::update() {

		//Entity* root = core->sceneManager->currentScene->root;
		//root->transform->children[0]->localPosition.x += 0.001f;
		//root->transform->children[0]->updateTransform();
	}

	void Game::destroy() {

		delete game;
	}
}