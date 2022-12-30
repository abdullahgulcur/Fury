#include "pch.h"
#include "scenefile.h"
#include "scene.h"
#include "core.h"

namespace Fury {

	SceneFile::SceneFile(const char* scenepath) {

	}

	SceneFile::~SceneFile() {

		GlewContext* glewContext = Core::instance->glewContext;
		FileSystem* fileSystem = Core::instance->fileSystem;
		File* file = fileSystem->sceneFileToFile[this];

		fileSystem->sceneFiles.erase(std::remove(fileSystem->sceneFiles.begin(), fileSystem->sceneFiles.end(), file), fileSystem->sceneFiles.end());
		fileSystem->fileToSceneFile.erase(file);
		fileSystem->sceneFileToFile.erase(this);
	}

	bool SceneFile::saveEntities(std::string filePath) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* SceneNode = doc.allocate_node(rapidxml::node_element, "Scene");
		SceneNode->append_attribute(doc.allocate_attribute("Name", doc.allocate_string(Core::instance->sceneManager->currentScene->name.c_str())));
		doc.append_node(SceneNode);

		rapidxml::xml_node<>* entitiesNode = doc.allocate_node(rapidxml::node_element, "Entities");
		SceneNode->append_node(entitiesNode);

		Transform* root = Core::instance->sceneManager->currentScene->root->transform;
		for (Transform* child : root->children)
			SceneFile::saveEntitiesRecursively(child, doc, entitiesNode);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(filePath);
		file_stored << doc;
		file_stored.close();
		doc.clear();
		return true;
	}

	void SceneFile::saveEntitiesRecursively(Transform* transform, rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entityNode) {

		rapidxml::xml_node<>* entity = doc.allocate_node(rapidxml::node_element, "Entity");
		entity->append_attribute(doc.allocate_attribute("Name", doc.allocate_string(transform->entity->name.c_str())));
		entity->append_attribute(doc.allocate_attribute("Id", doc.allocate_string(std::to_string(transform->entity->id).c_str())));

		saveTransformComponent(doc, entity, transform);

		//if (Light* lightComp = child->entity->getComponent<Light>())
		//	saveLightComponent(doc, entity, lightComp);

		if (MeshRenderer* meshRendererComp = transform->entity->getComponent<MeshRenderer>())
			saveMeshRendererComponent(doc, entity, meshRendererComp);

		//if (TerrainGenerator* terrainComp = child->entity->getComponent<TerrainGenerator>())
		//	saveTerrainGeneratorComponent(doc, entity, terrainComp);

		//if (Rigidbody* rigidbodyComp = child->entity->getComponent<Rigidbody>())
		//	saveRigidbodyComponent(doc, entity, rigidbodyComp);

		if (GameCamera* cameraComp = transform->entity->getComponent<GameCamera>())
			saveGameCameraComponent(doc, entity, cameraComp);

		//for (auto& comp : child->entity->getComponents<BoxCollider>())
		//	saveBoxColliderComponent(doc, entity, comp);

		//for (auto& comp : child->entity->getComponents<SphereCollider>())
		//	saveSphereColliderComponent(doc, entity, comp);

		//for (auto& comp : child->entity->getComponents<CapsuleCollider>())
		//	saveCapsuleColliderComponent(doc, entity, comp);

		//for (auto& comp : child->entity->getComponents<MeshCollider>())
		//	saveMeshColliderComponent(doc, entity, comp);

		entityNode->append_node(entity);

		for (Transform* child : transform->children)
			SceneFile::saveEntitiesRecursively(child, doc, entity);
	}

	void SceneFile::loadEntities(std::string filePath) {

		std::ifstream file(filePath);

		if (file.fail())
			return;

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* scene_node = NULL;
		rapidxml::xml_node<>* entities_node = NULL;

		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');

		doc.parse<0>(&buffer[0]);

		scene_node = doc.first_node("Scene");
		Core::instance->sceneManager->currentScene->name = scene_node->first_attribute("Name")->value();
		entities_node = scene_node->first_node("Entities");

		SceneFile::loadEntitiesRecursively(entities_node, Core::instance->sceneManager->currentScene->root);

		file.close();
	}

	void SceneFile::renameFile(std::string path, std::string name) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* sceneNode = doc.allocate_node(rapidxml::node_element, "Scene");
		sceneNode->append_attribute(doc.allocate_attribute("Name", name.c_str()));
		doc.append_node(sceneNode);

		rapidxml::xml_node<>* entitiesNode = doc.allocate_node(rapidxml::node_element, "Entities");
		sceneNode->append_node(entitiesNode);

		Transform* root = Core::instance->sceneManager->currentScene->root->transform;
		for (Transform* child : root->children)
			SceneFile::saveEntitiesRecursively(child, doc, entitiesNode);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(path);
		file_stored << doc;
		file_stored.close();
		doc.clear();
	}

	void SceneFile::loadEntitiesRecursively(rapidxml::xml_node<>* parentNode, Entity* parent) {

		for (rapidxml::xml_node<>* entityNode = parentNode->first_node("Entity"); entityNode; entityNode = entityNode->next_sibling()) {

			//Entity* ent = core->scene->newEntity(entityNode->first_attribute("Name")->value(), parent,
			//	atoi(entityNode->first_attribute("Id")->value()));
			unsigned int id = atoi(entityNode->first_attribute("Id")->value());
			Entity* ent = new Entity(entityNode->first_attribute("Name")->value(), parent, id);
			Core::instance->sceneManager->currentScene->entityIdToEntity.insert(std::pair<unsigned int, Entity*>(id, ent));

			if (id >= Core::instance->sceneManager->currentScene->idCounter)
				Core::instance->sceneManager->currentScene->idCounter = id + 1;

			SceneFile::loadTransformComponent(ent, entityNode);
			//SceneFile::loadLightComponent(ent, entityNode);
			SceneFile::loadMeshRendererComponent(ent, entityNode);
			//SceneFile::loadTerrainGeneratorComponent(ent, entityNode);
			//SceneFile::loadBoxColliderComponents(ent, entityNode);
			//SceneFile::loadSphereColliderComponents(ent, entityNode);
			//SceneFile::loadCapsuleColliderComponents(ent, entityNode);
			//SceneFile::loadMeshColliderComponents(ent, entityNode);
			//SceneFile::loadRigidbodyComponent(ent, entityNode);
			SceneFile::loadGameCameraComponent(ent, entityNode);

			SceneFile::loadEntitiesRecursively(entityNode, ent);
		}
	}

	// NOT TESTED !!!
	bool SceneFile::saveMeshRendererComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, MeshRenderer* meshRenderer) {

		rapidxml::xml_node<>* meshRendererNode = doc.allocate_node(rapidxml::node_element, "MeshRenderer");

		MeshFile* meshFile = meshRenderer->meshFile;
		if (Core::instance->fileSystem->meshFileToFile.find(meshFile) != Core::instance->fileSystem->meshFileToFile.end()) {
			File* file = Core::instance->fileSystem->meshFileToFile[meshFile];
			meshRendererNode->append_attribute(doc.allocate_attribute("MeshFileId", doc.allocate_string(std::to_string(file->id).c_str())));
		}
		else {
			meshRendererNode->append_attribute(doc.allocate_attribute("MeshFileId", doc.allocate_string(std::to_string(-1).c_str())));
		}

		MaterialFile* matFile = meshRenderer->materialFile;
		if (Core::instance->fileSystem->matFileToFile.find(matFile) != Core::instance->fileSystem->matFileToFile.end()) {
			File* file = Core::instance->fileSystem->matFileToFile[matFile];
			meshRendererNode->append_attribute(doc.allocate_attribute("MatFileId", doc.allocate_string(std::to_string(file->id).c_str())));
		}
		else {
			meshRendererNode->append_attribute(doc.allocate_attribute("MatFileId", doc.allocate_string(std::to_string(-1).c_str())));
		}

		entNode->append_node(meshRendererNode);
		return true;
	}

	// NOT TESTED !!!
	bool SceneFile::loadMeshRendererComponent(Entity* ent, rapidxml::xml_node<>* entNode) {

		FileSystem* fileSystem = Core::instance->fileSystem;
		rapidxml::xml_node<>* meshRendererNode = entNode->first_node("MeshRenderer");

		if (meshRendererNode == NULL)
			return false;

		MeshRenderer* meshRendererComp = ent->addComponent<MeshRenderer>();

		int meshFileId = atoi(meshRendererNode->first_attribute("MeshFileId")->value());
		auto file = fileSystem->files.find(meshFileId);
		if (file != fileSystem->files.end()) {

			auto meshFile = fileSystem->fileToMeshFile.find(file->second);
			if (meshFile != fileSystem->fileToMeshFile.end())
				meshRendererComp->setMeshFileOnLoad(meshFile->second);
			else
				meshRendererComp->meshFile = NULL;
		}
		else
			meshRendererComp->meshFile = NULL;

		int matFileId = atoi(meshRendererNode->first_attribute("MatFileId")->value());
		file = fileSystem->files.find(matFileId);
		if (file != fileSystem->files.end()) {

			auto matFile = fileSystem->fileToMatFile.find(file->second);
			if (matFile != fileSystem->fileToMatFile.end())
				meshRendererComp->setMatFileOnLoad(matFile->second);
			else
				meshRendererComp->materialFile = NULL;// fileSystem->pbrMaterialNoTexture;
		}
		else
			meshRendererComp->materialFile = NULL;// fileSystem->pbrMaterialNoTexture;

		return true;
	}

	bool SceneFile::saveTransformComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Transform* transform) {

		rapidxml::xml_node<>* transformNode = doc.allocate_node(rapidxml::node_element, "Transform");

		rapidxml::xml_node<>* positionNode = doc.allocate_node(rapidxml::node_element, "Position");
		positionNode->append_attribute(doc.allocate_attribute("X", doc.allocate_string(std::to_string(transform->localPosition.x).c_str())));
		positionNode->append_attribute(doc.allocate_attribute("Y", doc.allocate_string(std::to_string(transform->localPosition.y).c_str())));
		positionNode->append_attribute(doc.allocate_attribute("Z", doc.allocate_string(std::to_string(transform->localPosition.z).c_str())));
		transformNode->append_node(positionNode);

		rapidxml::xml_node<>* rotationNode = doc.allocate_node(rapidxml::node_element, "Rotation");
		rotationNode->append_attribute(doc.allocate_attribute("X", doc.allocate_string(std::to_string(transform->localRotation.x).c_str())));
		rotationNode->append_attribute(doc.allocate_attribute("Y", doc.allocate_string(std::to_string(transform->localRotation.y).c_str())));
		rotationNode->append_attribute(doc.allocate_attribute("Z", doc.allocate_string(std::to_string(transform->localRotation.z).c_str())));
		transformNode->append_node(rotationNode);

		rapidxml::xml_node<>* scaleNode = doc.allocate_node(rapidxml::node_element, "Scale");
		scaleNode->append_attribute(doc.allocate_attribute("X", doc.allocate_string(std::to_string(transform->localScale.x).c_str())));
		scaleNode->append_attribute(doc.allocate_attribute("Y", doc.allocate_string(std::to_string(transform->localScale.y).c_str())));
		scaleNode->append_attribute(doc.allocate_attribute("Z", doc.allocate_string(std::to_string(transform->localScale.z).c_str())));
		transformNode->append_node(scaleNode);

		entNode->append_node(transformNode);

		return true;
	}

	bool SceneFile::loadTransformComponent(Entity* ent, rapidxml::xml_node<>* entNode) {

		rapidxml::xml_node<>* transform_node = entNode->first_node("Transform");

		ent->transform->localPosition.x = atof(transform_node->first_node("Position")->first_attribute("X")->value());
		ent->transform->localPosition.y = atof(transform_node->first_node("Position")->first_attribute("Y")->value());
		ent->transform->localPosition.z = atof(transform_node->first_node("Position")->first_attribute("Z")->value());

		ent->transform->localRotation.x = atof(transform_node->first_node("Rotation")->first_attribute("X")->value());
		ent->transform->localRotation.y = atof(transform_node->first_node("Rotation")->first_attribute("Y")->value());
		ent->transform->localRotation.z = atof(transform_node->first_node("Rotation")->first_attribute("Z")->value());

		ent->transform->localScale.x = atof(transform_node->first_node("Scale")->first_attribute("X")->value());
		ent->transform->localScale.y = atof(transform_node->first_node("Scale")->first_attribute("Y")->value());
		ent->transform->localScale.z = atof(transform_node->first_node("Scale")->first_attribute("Z")->value());

		ent->transform->updateTransform();

		return true;
	}

	bool SceneFile::saveGameCameraComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, GameCamera* camera) {

		rapidxml::xml_node<>* camNode = doc.allocate_node(rapidxml::node_element, "GameCamera");
		camNode->append_attribute(doc.allocate_attribute("Projection", doc.allocate_string(std::to_string(camera->projectionType).c_str())));
		camNode->append_attribute(doc.allocate_attribute("FovAxis", doc.allocate_string(std::to_string(camera->fovAxis).c_str())));
		camNode->append_attribute(doc.allocate_attribute("Near", doc.allocate_string(std::to_string(camera->nearClip).c_str())));
		camNode->append_attribute(doc.allocate_attribute("Far", doc.allocate_string(std::to_string(camera->farClip).c_str())));
		camNode->append_attribute(doc.allocate_attribute("FOV", doc.allocate_string(std::to_string(camera->fov).c_str())));
		//camNode->append_attribute(doc.allocate_attribute("Width", doc.allocate_string(std::to_string(camera->width).c_str())));
		//camNode->append_attribute(doc.allocate_attribute("Height", doc.allocate_string(std::to_string(camera->height).c_str())));
		entNode->append_node(camNode);

		return true;
	}

	bool SceneFile::loadGameCameraComponent(Entity* ent, rapidxml::xml_node<>* entNode) {

		rapidxml::xml_node<>* cameraNode = entNode->first_node("GameCamera");

		if (cameraNode == NULL)
			return false;

		GameCamera* cameraComp = ent->addComponent<GameCamera>();
		cameraComp->projectionType = atoi(cameraNode->first_attribute("Projection")->value());
		cameraComp->fovAxis = atoi(cameraNode->first_attribute("FovAxis")->value());
		cameraComp->nearClip = atof(cameraNode->first_attribute("Near")->value());
		cameraComp->farClip = atof(cameraNode->first_attribute("Far")->value());
		cameraComp->fov = atof(cameraNode->first_attribute("FOV")->value());
	//	int x = atoi(cameraNode->first_attribute("Width")->value());
	//	int y = atoi(cameraNode->first_attribute("Height")->value());
		Core::instance->sceneManager->currentScene->primaryCamera = cameraComp;
		//cameraComp->setMatrices(ent->transform);

#ifdef EDITOR_MODE
		cameraComp->init(ent->transform, 1024, 768); // burasi degisecek, editor icin calisiyor sadece. game tarafi ? henuz netlik yok...
#else
		cameraComp->init(Core::instance->glfwContext->mode->width, Core::instance->glfwContext->mode->height);
#endif

		return true;
	}

}