#pragma once

#include "core.h"

#include "GLM/gtc/type_ptr.hpp"

#include "GLFW/glfw3.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"

#include "imguizmo/imguizmo.h"


// You probably can make all globals as local... Try to do it!

#define WHITE ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
#define DEFAULT_TEXT_COLOR ImVec4(0.8f, 0.8f, 0.8f, 1.0f)
#define TEXT_SELECTED_COLOR ImVec4(0.2f, 0.72f, 0.95f, 1.f)

namespace Fury {

	class Core;
	class File;
	class SceneFile;
	class Terrain;
	class GameCamera;
}

using namespace Fury;

namespace Editor {

	class Editor;

	class Menu {

	private:

		unsigned int folder64TextureId;
		unsigned int folderClosed16TextureId;
		unsigned int folderOpened16TextureId;
		unsigned int greaterTextureId;
		unsigned int stopTextureId;
		unsigned int startTextureId;
		unsigned int pauseTextureId;
		unsigned int sceneFileTextureId;
		unsigned int contextMenuTextureId;
		unsigned int meshColliderTextureId;
		unsigned int meshRendererTextureId;
		unsigned int transformTextureId;
		unsigned int cameraTextureId;

		Entity* holdedEntity = NULL;
		Entity* coloredEntity = NULL;

		// Current parent can be clicked from navigation pane of the file browser,
		// directory text and double click file in main pane.
		// It shows all the child files at main of the file browser.
		//File* currentParentFile = NULL;

		ImVec2 mousePosWhenClicked;

		// Current child is clicked from right part of file browser
		File* holdedFileMainPane = NULL;
		File* selectedFileMainPane = NULL;
		File* coloredFileMainPane = NULL;

		File* holdedFileNavigationPane = NULL;
		File* selectedFileNavigationPane = NULL;
		File* coloredFileNavigationPane = NULL;

		File* holdedFileDirectoryText = NULL;
		File* selectedFileDirectoryText = NULL;
		File* coloredFileDirectoryText = NULL;

		File* renameFile = NULL;

		Entity* renameEntity = NULL;
		Entity* toBeOpened = NULL;
		Entity* sceneClickEntity = NULL;

		bool inspectorHovered = false;
		bool folderLineClicked = false;
		bool anyChildFileHovered = false;
		bool anyEntityHovered = false;
		bool popupItemClicked = false; // entity kismi icin
		bool dragToMainPaneFile = false;

		/* when popup is open from main pane file */
		bool mainPaneFilePopup = false;

		ImGuizmo::OPERATION optype = ImGuizmo::OPERATION::TRANSLATE;
		//bool guizmoUsing = false;
		bool scenePanelClicked = false;

		bool filePanelClicked = false;

	public:

		Entity* selectedEntity = NULL;

		ImVec2 scenePos;
		ImVec2 sceneRegion;
		ImVec2 gameRegion;

		Menu();

		//void handleInputs();
		bool mouseMovedAfterHoldingFile();
		void init();
		void initImGui();
		void selectedNavigationFileDeletedControl();
		void inputControl();
		void newFrameImGui();
		void update();
		void renderImGui();
		void destroyImGui();
		void setTheme();
		void createPanels();
		void mainMenuBar();
		void secondaryMenuBar();

		//PANELS
		void createFilesPanel();
		void createStatisticsPanel();
		void createHierarchyPanel();
		void hiearchyCreateButton();
		void createSceneGraphRecursively(Transform& transform);

		void showCurrentDirectoryText();
		void createFoldersRecursively(File* file);
		void createFilesPanelRightPart(ImVec2 area);

		void createScenePanel();
		void createGamePanel();
		void createInspectorPanel();
		void showEntityName();
		void addComponentButton();
		void showTransformComponent(int index);
		void showMeshRendererComponent(int index, MeshRenderer* comp);
		void showTerrainComponent(int index, Terrain* comp);
		void showGameCameraComponent(GameCamera* camComp, int index);

		void showMaterialProperties(int index, MaterialFile* mat);
		void showTextureProperties();
		bool contextMenuPopup(Component* component, int index);
		unsigned int getTextureIdForFile(File* file);
		void initDefaultIcons();

	};
}

// menu de 721 deki satirlar oncekine benzetilecek (scene camera)
// scene deki primary camera, gamecamera component herhangi bir entity e eklendiginde set edilecek
// renderer de gamecamera fbo ya nesneler cizdirilecek
// editor acildiginda gamecamera da set edilmesi lazim
// data oriented bir sekilde frustum culling yapmali...