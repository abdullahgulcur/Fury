#include "pch.h"
#include "saveloadsystem.h"
#include "scene.h"
#include "core.h"

namespace Fury {

	SaveLoadSystem::SaveLoadSystem() {}

	bool SaveLoadSystem::saveEntities(Core* core) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* entitiesNode = doc.allocate_node(rapidxml::node_element, "Entities");
		doc.append_node(entitiesNode);

		Transform* root = core->sceneManager->currentScene->root->transform;
		for(Transform* child : root->children)
			SaveLoadSystem::saveEntitiesRecursively(child, doc, entitiesNode, core);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(core->fileSystem->getAssetsFileDBPath());
		file_stored << doc;
		file_stored.close();
		doc.clear();
		return true;
	}

	void SaveLoadSystem::saveEntitiesRecursively(Transform* transform, rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entityNode, Core* core) {

		rapidxml::xml_node<>* entity = doc.allocate_node(rapidxml::node_element, "Entity");
		entity->append_attribute(doc.allocate_attribute("Name", doc.allocate_string(transform->entity->name.c_str())));
		entity->append_attribute(doc.allocate_attribute("Id", doc.allocate_string(std::to_string(transform->entity->id).c_str())));

		saveTransformComponent(doc, entity, transform);

		//if (Light* lightComp = child->entity->getComponent<Light>())
		//	saveLightComponent(doc, entity, lightComp);

		if (MeshRenderer* meshRendererComp = transform->entity->getComponent<MeshRenderer>())
			saveMeshRendererComponent(doc, entity, meshRendererComp, core);

		//if (TerrainGenerator* terrainComp = child->entity->getComponent<TerrainGenerator>())
		//	saveTerrainGeneratorComponent(doc, entity, terrainComp);

		//if (Rigidbody* rigidbodyComp = child->entity->getComponent<Rigidbody>())
		//	saveRigidbodyComponent(doc, entity, rigidbodyComp);

		//if (GameCamera* cameraComp = child->entity->getComponent<GameCamera>())
		//	saveGameCameraComponent(doc, entity, cameraComp);

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
			SaveLoadSystem::saveEntitiesRecursively(child, doc, entity, core);
	}

	void SaveLoadSystem::loadEntities(Core* core, std::string filePath) {

		//std::ifstream file(core->fileSystem->getAssetsFileDBPath());
		std::ifstream file(filePath);

		if (file.fail())
			return;

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* root_node = NULL;

		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');

		doc.parse<0>(&buffer[0]);

		root_node = doc.first_node("Entities");

		//Entity* ent = new Entity("Root");
		//core->scene->root = ent;
		//ent->transform->updateSelfAndChild();

		SaveLoadSystem::loadEntitiesRecursively(root_node, core->sceneManager->currentScene->root, core);

		file.close();
	}

	void SaveLoadSystem::loadEntitiesRecursively(rapidxml::xml_node<>* parentNode, Entity* parent, Core* core) {

		for (rapidxml::xml_node<>* entityNode = parentNode->first_node("Entity"); entityNode; entityNode = entityNode->next_sibling()) {

			//Entity* ent = core->scene->newEntity(entityNode->first_attribute("Name")->value(), parent,
			//	atoi(entityNode->first_attribute("Id")->value()));
			unsigned int id = atoi(entityNode->first_attribute("Id")->value());
			Entity* ent = new Entity(entityNode->first_attribute("Name")->value(), parent, id);
			core->sceneManager->currentScene->entityIdToEntity.insert(std::pair<unsigned int, Entity*>(id, ent));

			if (id >= core->sceneManager->currentScene->idCounter)
				core->sceneManager->currentScene->idCounter = id + 1;

			SaveLoadSystem::loadTransformComponent(ent, entityNode);
			//SaveLoadSystem::loadLightComponent(ent, entityNode);
			SaveLoadSystem::loadMeshRendererComponent(ent, entityNode, core);
			//SaveLoadSystem::loadTerrainGeneratorComponent(ent, entityNode);
			//SaveLoadSystem::loadBoxColliderComponents(ent, entityNode);
			//SaveLoadSystem::loadSphereColliderComponents(ent, entityNode);
			//SaveLoadSystem::loadCapsuleColliderComponents(ent, entityNode);
			//SaveLoadSystem::loadMeshColliderComponents(ent, entityNode);
			//SaveLoadSystem::loadRigidbodyComponent(ent, entityNode);
			//SaveLoadSystem::loadGameCameraComponent(ent, entityNode);

			SaveLoadSystem::loadEntitiesRecursively(entityNode, ent, core);
		}
	}

	// NOT TESTED !!!
	bool SaveLoadSystem::saveMeshRendererComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, MeshRenderer* meshRenderer, Core* core) {

		rapidxml::xml_node<>* meshRendererNode = doc.allocate_node(rapidxml::node_element, "MeshRenderer");

		MeshFile* meshFile = meshRenderer->meshFile;
		if (core->fileSystem->meshFileToFile.find(meshFile) != core->fileSystem->meshFileToFile.end()) {
			File* file = core->fileSystem->meshFileToFile[meshFile];
			meshRendererNode->append_attribute(doc.allocate_attribute("MeshFileId", doc.allocate_string(std::to_string(file->id).c_str())));
		}

		MaterialFile* matFile = meshRenderer->materialFile;
		if (core->fileSystem->matFileToFile.find(matFile) != core->fileSystem->matFileToFile.end()) {
			File* file = core->fileSystem->matFileToFile[matFile];
			meshRendererNode->append_attribute(doc.allocate_attribute("MatFileId", doc.allocate_string(std::to_string(file->id).c_str())));
		}
		entNode->append_node(meshRendererNode);
		return true;
	}

	// NOT TESTED !!!
	bool SaveLoadSystem::loadMeshRendererComponent(Entity* ent, rapidxml::xml_node<>* entNode, Core* core) {

		rapidxml::xml_node<>* meshRendererNode = entNode->first_node("MeshRenderer");

		if (meshRendererNode == NULL)
			return false;

		MeshRenderer* meshRendererComp = ent->addComponent<MeshRenderer>();

		auto file = core->fileSystem->files.find(atoi(meshRendererNode->first_attribute("MeshFileId")->value()));
		if (file != core->fileSystem->files.end()) {

			auto meshFile = core->fileSystem->fileToMeshFile.find(file->second);
			if (meshFile != core->fileSystem->fileToMeshFile.end())
				meshRendererComp->meshFile = meshFile->second;
			else
				meshRendererComp->meshFile = NULL;
		}
		else
			meshRendererComp->meshFile = NULL;

		file = core->fileSystem->files.find(atoi(meshRendererNode->first_attribute("MatFileId")->value()));
		if (file != core->fileSystem->files.end()) {

			auto matFile = core->fileSystem->fileToMatFile.find(file->second);
			if (matFile != core->fileSystem->fileToMatFile.end())
				meshRendererComp->materialFile = matFile->second;
			else
				meshRendererComp->materialFile = NULL;// core->fileSystem->pbrMaterialNoTexture;
		}
		else
			meshRendererComp->materialFile = NULL;//core->fileSystem->pbrMaterialNoTexture;
		
		return true;
	}

	//bool SaveLoadSystem::saveSceneCamera(SceneCamera* sceneCamera) {

	//	rapidxml::xml_document<> doc;
	//	rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
	//	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	//	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	//	doc.append_node(decl);

	//	rapidxml::xml_node<>* cameraNode = doc.allocate_node(rapidxml::node_element, "SceneCamera");
	//	doc.append_node(cameraNode);

	//	rapidxml::xml_node<>* positionNode = doc.allocate_node(rapidxml::node_element, "Position");
	//	positionNode->append_attribute(doc.allocate_attribute("X", doc.allocate_string(std::to_string(editor->sceneCamera->position.x).c_str())));
	//	positionNode->append_attribute(doc.allocate_attribute("Y", doc.allocate_string(std::to_string(editor->sceneCamera->position.y).c_str())));
	//	positionNode->append_attribute(doc.allocate_attribute("Z", doc.allocate_string(std::to_string(editor->sceneCamera->position.z).c_str())));
	//	cameraNode->append_node(positionNode);

	//	rapidxml::xml_node<>* angleNode = doc.allocate_node(rapidxml::node_element, "Angle");
	//	angleNode->append_attribute(doc.allocate_attribute("Horizontal", doc.allocate_string(std::to_string(editor->sceneCamera->horizontalAngle).c_str())));
	//	angleNode->append_attribute(doc.allocate_attribute("Vertical", doc.allocate_string(std::to_string(editor->sceneCamera->verticalAngle).c_str())));
	//	cameraNode->append_node(angleNode);

	//	std::string xml_as_string;
	//	rapidxml::print(std::back_inserter(xml_as_string), doc);

	//	std::ofstream file_stored(editor->fileSystem->assetsPathExternal + "\\MyProject\\Database\\scenecamera_db.xml");
	//	file_stored << doc;
	//	file_stored.close();
	//	doc.clear();
	//	return true;
	//}

	//bool SaveLoadSystem::loadSceneCamera(SceneCamera* sceneCamera) {

	//	std::ifstream file(editor->fileSystem->assetsPathExternal + "\\MyProject\\Database\\scenecamera_db.xml");

	//	if (file.fail())
	//		return false;

	//	rapidxml::xml_document<> doc;
	//	rapidxml::xml_node<>* root_node = NULL;

	//	std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	//	buffer.push_back('\0');

	//	doc.parse<0>(&buffer[0]);

	//	root_node = doc.first_node("SceneCamera");

	//	editor->sceneCamera->position.x = atof(root_node->first_node("Position")->first_attribute("X")->value());
	//	editor->sceneCamera->position.y = atof(root_node->first_node("Position")->first_attribute("Y")->value());
	//	editor->sceneCamera->position.z = atof(root_node->first_node("Position")->first_attribute("Z")->value());

	//	editor->sceneCamera->horizontalAngle = atof(root_node->first_node("Angle")->first_attribute("Horizontal")->value());
	//	editor->sceneCamera->verticalAngle = atof(root_node->first_node("Angle")->first_attribute("Vertical")->value());

	//	file.close();
	//	return true;
	//}

	bool SaveLoadSystem::saveTransformComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Transform* transform) {

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

	bool SaveLoadSystem::loadTransformComponent(Entity* ent, rapidxml::xml_node<>* entNode) {

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
}

