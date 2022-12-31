#include "pch.h"
#include "entrypoint.h"
#include "scene.h"
#include "input.h"

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

		Entity* root = core->sceneManager->currentScene->root;
	}

	void Game::update() {

		Entity* root = core->sceneManager->currentScene->root;

		if (Input::getKeyDown(KeyCode::W))
			root->transform->children[0]->localPosition.z += 0.01f;

		if (Input::getKeyDown(KeyCode::A))
			root->transform->children[0]->localPosition.x += 0.01f;

		if (Input::getKeyDown(KeyCode::D))
			root->transform->children[0]->localPosition.x -= 0.01f;

		if (Input::getKeyDown(KeyCode::S))
			root->transform->children[0]->localPosition.z -= 0.01f;

		root->transform->children[0]->updateTransform();
	}

	void Game::destroy() {

		delete game;
	}
}