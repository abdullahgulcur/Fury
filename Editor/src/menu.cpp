#include "pch.h"
#include "core.h"
#include "editor.h"
#include "menu.h"
#include "entity.h"
#include "component/meshrenderer.h"
#include "component/terrain.h"
#include "component/gamecamera.h"
#include "filesystem/materialfile.h"
#include "filesystem/scenefile.h"
#include "scene.h"

namespace Editor {

	bool Menu::mouseMovedAfterHoldingFile() {

		float dx = mousePosWhenClicked.x - ImGui::GetMousePos().x;
		float dy = mousePosWhenClicked.y - ImGui::GetMousePos().y;

		if (sqrt(dx * dx + dy * dy) < 2.0f)
			return false;

		return true;
	}

	Menu::Menu() {

	}

	void Menu::init() {

		Menu::initImGui();

		selectedFileNavigationPane = Core::instance->fileSystem->rootFile;
		coloredFileNavigationPane = Core::instance->fileSystem->rootFile;

		Menu::initDefaultIcons();
	}

	void Menu::initImGui() {

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			Menu::setTheme();

		ImGui_ImplGlfw_InitForOpenGL(Core::instance->glfwContext->GLFW_window, true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void Menu::newFrameImGui() {

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void Menu::inputControl() {

		if (!ImGui::GetIO().KeyCtrl) {

			if (ImGui::IsKeyPressed('T'))
				optype = ImGuizmo::OPERATION::TRANSLATE;

			if (ImGui::IsKeyPressed('R'))
				optype = ImGuizmo::OPERATION::ROTATE;

			if (ImGui::IsKeyPressed('S'))
				optype = ImGuizmo::OPERATION::SCALE;
		}

		if (ImGui::IsMouseReleased(0)) {

			if (holdedFileMainPane) {

				if (!Menu::mouseMovedAfterHoldingFile()) {
					selectedFileMainPane = holdedFileMainPane;
					selectedEntity = NULL;
					coloredEntity = NULL;
				}
				else {
					coloredFileMainPane = selectedFileMainPane;
				}

				holdedFileMainPane = NULL;
			}

			if (holdedFileNavigationPane) {

				if (!Menu::mouseMovedAfterHoldingFile()) {
					selectedFileNavigationPane = holdedFileNavigationPane;

					Core::instance->fileSystem->currentOpenFile = selectedFileNavigationPane;
				}
				else {
					coloredFileNavigationPane = selectedFileNavigationPane;
				}

				holdedFileNavigationPane = NULL;
			}

			if (holdedFileDirectoryText) {

				if (!Menu::mouseMovedAfterHoldingFile()) {
					selectedFileDirectoryText = holdedFileDirectoryText;
					selectedFileNavigationPane = selectedFileDirectoryText;
					coloredFileNavigationPane = selectedFileNavigationPane;

					Core::instance->fileSystem->currentOpenFile = selectedFileNavigationPane;
				}
				else {
					coloredFileDirectoryText = selectedFileDirectoryText;
				}

				holdedFileDirectoryText = NULL;
			}

			if (holdedEntity) {
				selectedEntity = holdedEntity;
				holdedEntity = NULL;
			}
		}

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)) || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_KeyPadEnter))) {

			if (selectedFileMainPane != NULL && renameFile == NULL && selectedFileMainPane->parent == selectedFileNavigationPane) {

				if (selectedFileMainPane->type == FileType::folder) {
					selectedFileNavigationPane = selectedFileMainPane;

					Core::instance->fileSystem->currentOpenFile = selectedFileNavigationPane;
				}
				else
					ShellExecute(NULL, L"open", std::filesystem::absolute(selectedFileMainPane->path).c_str(), NULL, NULL, SW_RESTORE);
			}

			if (renameFile != NULL) renameFile = NULL;
		}

		if (ImGui::IsMouseDoubleClicked(0)) {

			if (selectedFileMainPane) {
				if (selectedFileMainPane->type == FileType::folder) {
					selectedFileNavigationPane = selectedFileMainPane;
					coloredFileNavigationPane = selectedFileMainPane;

					Core::instance->fileSystem->currentOpenFile = selectedFileNavigationPane;
				}
				else {

					if (selectedFileMainPane->type == FileType::scene) {
						Core::instance->sceneManager->loadScene(selectedFileMainPane->name);
						Core::instance->sceneManager->saveSceneManagerFile();
					}
					else
						ShellExecute(NULL, L"open", std::filesystem::absolute(selectedFileMainPane->path).c_str(), NULL, NULL, SW_RESTORE);
				}
			}
		}

		if (scenePanelClicked) {

			if (!ImGuizmo::IsUsing()) {

				if (sceneClickEntity) {
					selectedEntity = sceneClickEntity;
					coloredEntity = selectedEntity;
				}
				else {
					selectedEntity = NULL;
					coloredEntity = NULL;
				}
			}

			sceneClickEntity = NULL;
			scenePanelClicked = false;
		}

		if (ImGui::GetIO().KeyCtrl) {

			if (ImGui::IsKeyPressed('D')) {

				if (selectedFileMainPane != NULL && renameFile == NULL && selectedFileMainPane->parent == selectedFileNavigationPane)
					Core::instance->fileSystem->duplicateFile(selectedFileMainPane);

				if (selectedEntity != NULL)
					selectedEntity = Core::instance->sceneManager->currentScene->duplicate(selectedEntity);
			}

			if (ImGui::IsKeyPressed('S')) {

				auto sceneFile = Core::instance->fileSystem->fileToSceneFile[Core::instance->sceneManager->currentSceneFile];
				sceneFile->saveEntities(Core::instance->sceneManager->currentSceneFile->path);
			}

		}

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete))) {

			if (selectedEntity) {
				selectedEntity->destroy();
				selectedEntity = NULL;
			}
		}
	}

	void Menu::update() {

		Menu::selectedNavigationFileDeletedControl();

		Menu::newFrameImGui();
		Menu::createPanels();
		Menu::renderImGui();

		Menu::inputControl();
		Menu::deleteComponentAfterRender();
	}

	void Menu::deleteComponentAfterRender() {

		if (!deleteBuffer)
			return;

		selectedEntity->removeComponent(deleteBuffer);
		deleteBuffer = NULL;
	}

	void Menu::renderImGui() {

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void Menu::destroyImGui() {

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Menu::setTheme()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
		//colors[ImGuiCol_ChildBg] = ImVec4(0.9f, 0.00f, 0.00f,1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.8f, 0.8f, 0.8f, 1.f);
		colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.8f, 0.8f, 0.8f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.50f, 0.50f, 0.15f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
		colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(8.00f, 8.00f);
		style.FramePadding = ImVec2(5.00f, 2.00f);
		style.CellPadding = ImVec2(6.00f, 6.00f);
		style.ItemSpacing = ImVec2(6.00f, 6.00f);
		style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
		style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
		style.ScrollbarSize = 15;
		style.GrabMinSize = 10;
		style.WindowBorderSize = 0;
		style.ChildBorderSize = 1;
		style.PopupBorderSize = 1;
		style.FrameBorderSize = 1;
		style.TabBorderSize = 1;
		style.WindowRounding = 0;
		style.ChildRounding = 4;
		style.FrameRounding = 4;
		style.PopupRounding = 2;
		style.ScrollbarRounding = 9;
		style.GrabRounding = 3;
		style.LogSliderDeadzone = 4;
		style.TabRounding = 4;
		style.IndentSpacing = 20;
	}

	void Menu::selectedNavigationFileDeletedControl() {

		if (File* file = Core::instance->fileSystem->deletedFileParent) {
			selectedFileNavigationPane = file;
			coloredFileNavigationPane = file;
			Core::instance->fileSystem->deletedFileParent = NULL;
		}
	}

	void Menu::createPanels() {

		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking; //ImGuiWindowFlags_MenuBar

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		bool p_open = true;

		ImGui::Begin("EditorDockSpace", &p_open, window_flags);
		ImGui::PopStyleVar(1);

		ImGuiIO& io = ImGui::GetIO();

		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		}

		Menu::mainMenuBar();
		Menu::secondaryMenuBar();
		Menu::createInspectorPanel();
		Menu::createStatisticsPanel();
		//Menu::createAppPanel();
		Menu::createHierarchyPanel();
		Menu::createFilesPanel();
		Menu::createScenePanel();
		//Menu::createConsolePanel();
		Menu::createGamePanel();

		//Menu::handleInputs();

		ImGui::End();
	}

	void Menu::createStatisticsPanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
		ImGui::Begin("Statistics");

		ImGui::TextColored(DEFAULT_TEXT_COLOR, "%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::TextColored(DEFAULT_TEXT_COLOR, " Draw Calls %d", Core::instance->renderer->drawCallCount);

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void Menu::createScenePanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Scene");

		scenePos = ImGui::GetCursorScreenPos();
		ImVec2 content = ImGui::GetContentRegionAvail();

		ImGui::Image((ImTextureID)Editor::instance->sceneCamera->textureBuffer, content, ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {

			scenePanelClicked = true;

			selectedFileMainPane = NULL;
			coloredFileMainPane = NULL;

			ImVec2 mousePos = ImGui::GetMousePos();
			float mX = mousePos.x < scenePos.x || mousePos.x > scenePos.x + sceneRegion.x ? 0 : mousePos.x - scenePos.x;
			float mY = mousePos.y < scenePos.y || mousePos.y > scenePos.y + sceneRegion.y ? 0 : mousePos.y - scenePos.y;

			if (mX != 0 && mY != 0)
				sceneClickEntity = Editor::instance->renderer->detectAndGetEntityId(mX, sceneRegion.y - mY);
		}

		if (content.x != sceneRegion.x || content.y != sceneRegion.y) {

			Editor::instance->sceneCamera->aspectRatio = content.x / content.y;
			sceneRegion = content;
			Editor::instance->sceneCamera->recreateFBO((int)content.x, (int)content.y);
		}

		if (selectedEntity) {

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			float windowWidth = (float)ImGui::GetWindowWidth();
			float windowHeight = (float)ImGui::GetWindowHeight();

			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
			glm::mat4& model = selectedEntity->transform->model;

			ImGuizmo::Manipulate(glm::value_ptr(Editor::instance->sceneCamera->ViewMatrix), glm::value_ptr(Editor::instance->sceneCamera->ProjectionMatrix),
				optype, ImGuizmo::LOCAL, glm::value_ptr(model));

			if(ImGuizmo::IsUsing())
				selectedEntity->transform->updateTransformUsingGuizmo();
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}

	void Menu::createGamePanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Game");

		ImVec2 content = ImGui::GetContentRegionAvail();

		unsigned int textureId = 0;
		if (Core::instance->sceneManager->currentScene && Core::instance->sceneManager->currentScene->primaryCamera)
			textureId = Core::instance->sceneManager->currentScene->primaryCamera->textureBuffer;
		//unsigned int textureId = Core::instance->scene->primaryCamera && Core::instance->scene ? Core::instance->scene->primaryCamera->textureBuffer : 0;
		ImGui::Image((ImTextureID)textureId, content, ImVec2(0, 1), ImVec2(1, 0));

		if (content.x != gameRegion.x || content.y != gameRegion.y) {

			Core::instance->renderer->width = content.x;
			Core::instance->renderer->height = content.y;
			gameRegion = content;

			if(Core::instance->sceneManager->currentScene && Core::instance->sceneManager->currentScene->primaryCamera)
				Core::instance->sceneManager->currentScene->primaryCamera->resetResolution(content.x, content.y);
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}

	void Menu::createHierarchyPanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
		ImGui::Begin("Hierarchy");
		ImGui::PopStyleVar();

		Menu::hiearchyCreateButton();

		if (Core::instance->sceneManager->currentScene) {

			ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.f);
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

			ImVec2 size = ImGui::GetWindowSize();
			ImVec2 scrolling_child_size = ImVec2(size.x - 10, size.y - 54);
			ImGui::BeginChild("scrolling", scrolling_child_size, true, ImGuiWindowFlags_HorizontalScrollbar);

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {

					if (!anyEntityHovered && !popupItemClicked) {
						selectedEntity = NULL;
						coloredEntity = NULL;
					}
					selectedFileMainPane = NULL;
					coloredFileMainPane = NULL;
				}
			}

			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 5));

			ImGui::SetNextItemOpen(true);

			bool treeNodeOpen = ImGui::TreeNode("##0");

			ImVec2 imgsize = ImVec2(16.0f, 16.0f);
			ImVec2 uv0 = ImVec2(0.0f, 0.0f);
			ImVec2 uv1 = ImVec2(1.0f, 1.0f);
			ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

			ImGui::SameLine(20);
			//ImGui::Image((ImTextureID)0, imgsize, uv0, uv1, tint_col, border_col); //editorIcons.eyeTextureID
			//ImGui::SameLine();
			ImGui::TextColored(DEFAULT_TEXT_COLOR, Core::instance->sceneManager->currentScene->name.c_str());

			anyEntityHovered = false;
			popupItemClicked = false;
			if (treeNodeOpen)
			{
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_TREENODE_ENTITY"))
					{
						IM_ASSERT(payload->DataSize == sizeof(Entity*));
						Entity* payload_n = *(Entity**)payload->Data;
						Core::instance->sceneManager->currentScene->root->attachEntity(payload_n);
					}
					ImGui::EndDragDropTarget();
				}
				std::vector<Transform*>& children = Core::instance->sceneManager->currentScene->root->transform->children;
				for (int i = 0; i < children.size(); i++)
					Menu::createSceneGraphRecursively(*children[i]);
				ImGui::TreePop();
			}

			ImGui::EndChild();
		}
		ImGui::End();
	}

	void Menu::createSceneGraphRecursively(Transform& transform) {

		ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

		bool hasChildren = transform.entity->hasAnyChild();
		if (!hasChildren)
			node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		else
			node_flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;

		if (toBeOpened == transform.entity) {

			ImGui::SetNextItemOpen(true);
			toBeOpened = NULL;
		}
		bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)&transform, node_flags, "");

		if (ImGui::IsItemHovered())
			anyEntityHovered = true;

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {

			if (!ImGui::IsItemToggledOpen()) {
				holdedEntity = transform.entity;
				coloredEntity = holdedEntity;
			}
		}

		ImGui::PushID(transform.entity);
		ImGui::SetNextWindowSize(ImVec2(210, 95));

		if (ImGui::BeginPopupContextItem("scene_graph_popup"))
		{
			selectedEntity = transform.entity;
			coloredEntity = transform.entity;

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

			if (ImGui::Selectable("   Create Empty")) {

				Entity* entity = Core::instance->sceneManager->currentScene->newEntity("Entity", transform.entity);
				selectedEntity = entity;
				coloredEntity = entity;
				toBeOpened = entity->transform->parent->entity;
				popupItemClicked = true;

				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Rename")) {

				renameEntity = transform.entity;

				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			if (ImGui::Selectable("   Duplicate")) {

				selectedEntity = Core::instance->sceneManager->currentScene->duplicate(transform.entity);

				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			if (ImGui::Selectable("   Delete")) {

				delete selectedEntity;
				selectedEntity = NULL;

				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}
		ImGui::PopID();

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("_TREENODE_ENTITY", &transform.entity, sizeof(Entity*));
			ImGui::TextColored(DEFAULT_TEXT_COLOR, transform.entity->name.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_TREENODE_ENTITY"))
			{
				IM_ASSERT(payload->DataSize == sizeof(Entity*));
				Entity* payload_n = *(Entity**)payload->Data;
				if (transform.entity->attachEntity(payload_n)) {
					toBeOpened = transform.entity;
					return;
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();

		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

		//ImGui::Image((ImTextureID)0, size, uv0, uv1, tint_col, border_col); //editorIcons.gameObjectTextureID

		//ImGui::SameLine();

		//if (fileSystemControlVars.lastSelectedItemID == transform->children[i]->id)
		//	editorColors.textColor = TEXT_SELECTED_COLOR;
		//else
		//	editorColors.textColor = editorColors.textUnselectedColor;

		ImVec2 pos = ImGui::GetCursorPos();

		if (renameEntity == transform.entity) {

			char name[32];
			strcpy(name, (char*)transform.entity->name.c_str());
			ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
			ImGui::SetKeyboardFocusHere(0);
			ImGui::SetCursorPos(ImVec2(pos.x - 5, pos.y - 2));
			int length = IM_ARRAYSIZE(name);
			if (ImGui::InputText("##0", name, length, input_text_flags)) {
				if (length != 0)
					transform.entity->rename(name);
				renameEntity = NULL;
			}
		}
		else {

			if (coloredEntity == transform.entity)
				ImGui::TextColored(TEXT_SELECTED_COLOR, transform.entity->name.c_str());
			else
				ImGui::TextColored(DEFAULT_TEXT_COLOR, transform.entity->name.c_str());
		}

		//for (Transform* transform : (&transform)->children) { // Bu sekilde neden hata veriyor???
		//
		//	ImGui::Indent(15);
		//	if(nodeOpen)
		//		Menu::createSceneGraphRecursively(*transform);
		//	ImGui::Unindent(15);
		//}

		for (int i = 0; i < transform.children.size(); i++) {
			ImGui::Indent(15);
			if(nodeOpen)
				Menu::createSceneGraphRecursively(*transform.children[i]);
			ImGui::Unindent(15);
		}
	}

	void Menu::hiearchyCreateButton() {

		if (ImGui::Button("Create", ImVec2(60, 20)))
			ImGui::OpenPopup("context_menu_scene_hierarchy_popup");

		ImGui::SetNextWindowSize(ImVec2(210, 100));

		if (ImGui::BeginPopup("context_menu_scene_hierarchy_popup"))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

			if (ImGui::Selectable("   Entity")) {

				Entity* entity = Core::instance->sceneManager->currentScene->newEntity("Entity", Core::instance->sceneManager->currentScene->root);
				selectedEntity = entity;
				coloredEntity = entity;
				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				return;
			}

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Sun")) {

				//lastSelectedEntity = editor->scene->newLight(editor->scene->entities[0], "Sun", LightType::DirectionalLight);
				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				return;
			}

			if (ImGui::Selectable("   Point Light")) {

				//lastSelectedEntity = editor->scene->newLight(editor->scene->entities[0], "Light", LightType::PointLight);
				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				return;
			}

			p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Scene")) {

				//editor->addScene(Scene());

				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				return;
			}

			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}

	}

	void Menu::mainMenuBar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 4));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				//ShowExampleMenuFile();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help"))
			{
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	void Menu::secondaryMenuBar()
	{
		ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
		float height = ImGui::GetFrameHeight();

		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.13f, 0.13f, 0.13f, 1.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 6));
		if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height + 6, window_flags)) {
			if (ImGui::BeginMenuBar()) {

				float width = ImGui::GetWindowSize().x;
				ImVec2 pos = ImGui::GetCursorPos();
				ImGui::SetCursorPos(ImVec2(pos.x + width / 2 - 20, pos.y + 5));

				int frame_padding = 1;
				ImVec2 size = ImVec2(16.0f, 16.0f);
				ImVec2 uv0 = ImVec2(0.0f, 0.0f);
				ImVec2 uv1 = ImVec2(1.0f, 1.0f);
				ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

				if (!Editor::instance->gameStarted) {

					if (ImGui::ImageButton((ImTextureID)startTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {

						//Core::instance->sceneManager->currentScene->backup();
						auto file = Core::instance->fileSystem->sceneFileToFile.find(Core::instance->fileSystem->currentSceneFile);
						if (file != Core::instance->fileSystem->sceneFileToFile.end())
							Core::instance->fileSystem->currentSceneFile->saveEntities(file->second->path);

						Editor::instance->gameStarted = true;
						Core::instance->startGame();
					}
				}
				else {

					if (ImGui::ImageButton((ImTextureID)stopTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {

						int selectedEntityId = -1;
						if(selectedEntity)
							selectedEntityId = selectedEntity->id;

						Editor::instance->gameStarted = false;
						Core::instance->sceneManager->restartCurrentScene();

						if (selectedEntityId != -1) {
							selectedEntity = Core::instance->sceneManager->currentScene->entityIdToEntity[selectedEntityId];
							coloredEntity = selectedEntity;
						}
					}
				}
				ImGui::SameLine();

				if (ImGui::ImageButton((ImTextureID)pauseTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {

				}

				ImGui::EndMenuBar();
			}
			ImGui::End();
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();


		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 4));
		if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height + 4, window_flags)) {
			if (ImGui::BeginMenuBar()) {
				//ImGui::Text(statusMessage.c_str());
				ImGui::EndMenuBar();
			}
			ImGui::End();
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	void Menu::createFilesPanel()
	{
		static float lastMouseX;
		float mouseX = ImGui::GetMousePos().x;
		float mouseDeltaX = mouseX - lastMouseX;
		lastMouseX = mouseX;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

		ImGui::Begin("Files");

		ImGui::PopStyleVar();

		ImGui::Button("Create", ImVec2(60, 20));

		static int foldersPanelLeftPartWidth = 260;
		ImGui::SameLine(foldersPanelLeftPartWidth + 23);

		Menu::showCurrentDirectoryText();

		ImVec2 size = ImGui::GetWindowSize();
		ImVec2 scrolling_child_size = ImVec2(foldersPanelLeftPartWidth, size.y - 54);

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

		ImGui::BeginChild("scrolling", scrolling_child_size, true, ImGuiWindowFlags_HorizontalScrollbar);

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(2);

		ImGui::SetNextItemOpen(true);

		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 5));

		File* root = Core::instance->fileSystem->rootFile;

		ImGui::SetNextItemOpen(true);
		bool treeNodeOpen = ImGui::TreeNode("##0");

		ImVec2 imgsize = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

		ImGui::SameLine(20);
		ImGui::Image((ImTextureID)folderOpened16TextureId, imgsize, uv0, uv1, tint_col, border_col);
		ImGui::SameLine();

		if (coloredFileNavigationPane == root)
			ImGui::TextColored(TEXT_SELECTED_COLOR, root->name.c_str());
		else
			ImGui::TextColored(DEFAULT_TEXT_COLOR, root->name.c_str());

		if (treeNodeOpen)
		{
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) { //  || ImGui::IsItemClicked(ImGuiMouseButton_Right)

				holdedFileNavigationPane = root;
				coloredFileNavigationPane = holdedFileNavigationPane;
				mousePosWhenClicked = ImGui::GetMousePos();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_NAVIGATION_PANE_FILE"))
				{
					// ok
					IM_ASSERT(payload->DataSize == sizeof(File*));
					File* payload_n = *(File**)payload->Data;
					Core::instance->fileSystem->moveFile(payload_n, root);

					selectedFileMainPane = NULL;
					coloredFileMainPane = NULL;

					if (payload_n == selectedFileNavigationPane) {
						selectedFileNavigationPane = root;
						coloredFileNavigationPane = root;

						Core::instance->fileSystem->currentOpenFile = selectedFileNavigationPane;
					}
				}

				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_MAIN_PANE_FILE"))
				{
					//ok
					IM_ASSERT(payload->DataSize == sizeof(File*));
					File* payload_n = *(File**)payload->Data;
					Core::instance->fileSystem->moveFile(payload_n, root);

					selectedFileMainPane = NULL;
					coloredFileMainPane = NULL;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::Unindent(ImGui::GetStyle().IndentSpacing);

			std::vector<File*>& files = root->subfiles;
			for (int i = 0; i < files.size(); i++)
				Menu::createFoldersRecursively(files[i]);

			ImGui::TreePop();
		}

		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::Button(" ", ImVec2(3, ImGui::GetWindowSize().y - 53));
		if (ImGui::IsItemClicked())
			folderLineClicked = true;
		if (size.x > 400) {
			if (folderLineClicked) {
				if (foldersPanelLeftPartWidth > 200)
					foldersPanelLeftPartWidth += mouseDeltaX / 2;
				else
					foldersPanelLeftPartWidth = 200;

				if (size.x - foldersPanelLeftPartWidth >= 200)
					foldersPanelLeftPartWidth += mouseDeltaX / 2;
				else
					foldersPanelLeftPartWidth = size.x - 200;
			}
		}
		else
			foldersPanelLeftPartWidth = size.x / 2;

		ImGui::SameLine();
		ImVec2 scrolling_child_size_r = ImVec2(size.x - scrolling_child_size.x - 26, ImGui::GetWindowSize().y - 54);

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
		ImGui::BeginChild("scrolling_right", scrolling_child_size_r, true, ImGuiWindowFlags_HorizontalScrollbar);
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None)) {

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				
				if (!anyChildFileHovered) {

					coloredEntity = NULL;
					selectedEntity = NULL;

					selectedFileMainPane = NULL;
					coloredFileMainPane = NULL;
				}
			}
		}
		pos = ImGui::GetCursorPos();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);

		Menu::createFilesPanelRightPart(scrolling_child_size_r);

		ImGui::SetCursorPos(pos);
		ImGui::InvisibleButton("invsibleButton", scrolling_child_size_r);

		if (!dragToMainPaneFile) {
			if (ImGui::BeginDragDropTarget()) {

				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_NAVIGATION_PANE_FILE"))
				{
					// ok
					IM_ASSERT(payload->DataSize == sizeof(File*));
					File* payload_n = *(File**)payload->Data;
					Core::instance->fileSystem->moveFile(payload_n, selectedFileNavigationPane);
				}
			}
		}
		
		if (!mainPaneFilePopup) {
			if (ImGui::BeginPopupContextItem("scene_graph_popup")) {

				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

				if (ImGui::Selectable("   New Folder"))
					Core::instance->fileSystem->newFolder(selectedFileNavigationPane, "Folder");

				ImVec2 p = ImGui::GetCursorScreenPos();
				ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
				ImGui::Dummy(ImVec2(0, 1));

				if (ImGui::Selectable("   New PBR Material"))
					Core::instance->fileSystem->createPBRMaterialFile(selectedFileNavigationPane, "Material");

				if (ImGui::Selectable("   New Physics Material")) {


				}

				if (ImGui::Selectable("   New Scene")) {
					Core::instance->fileSystem->newScene(selectedFileNavigationPane, "Scene");

				}
				ImGui::PopStyleColor();
				ImGui::EndPopup();
			}
		}
		
		ImGui::EndChild();
		ImGui::End();
	}

	void Menu::createFoldersRecursively(File* file) {

		if (file->type != FileType::folder)
			return;

		ImGui::Indent(ImGui::GetStyle().IndentSpacing);

		ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

		bool hasSubFolder = Core::instance->fileSystem->hasSubFolder(file);
		if (!hasSubFolder)
			node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		else
			node_flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;

		ImGui::PushID(file->id);
		bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)file, node_flags, "");

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) { //  || ImGui::IsItemClicked(ImGuiMouseButton_Right)

			if (!ImGui::IsItemToggledOpen()) {
				holdedFileNavigationPane = file;
				coloredFileNavigationPane = holdedFileNavigationPane;
				mousePosWhenClicked = ImGui::GetMousePos();
			}
		}

		if (ImGui::BeginPopupContextItem("navigation_pane_file_popup"))
		{
			coloredFileNavigationPane = file;
			selectedFileNavigationPane = file;

			Core::instance->fileSystem->currentOpenFile = selectedFileNavigationPane;

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

			if (ImGui::Selectable("   Open")) {

				ShellExecute(NULL, L"open", std::filesystem::absolute(selectedFileNavigationPane->path).c_str(), NULL, NULL, SW_RESTORE);

				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Delete")) {

				File* parent = selectedFileNavigationPane->parent;
				Core::instance->fileSystem->deleteFileCompletely(selectedFileNavigationPane);
				selectedFileNavigationPane = parent;
				coloredFileNavigationPane = parent;

				Core::instance->fileSystem->currentOpenFile = selectedFileNavigationPane;

				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			if (ImGui::Selectable("   Duplicate")) {

				Core::instance->fileSystem->duplicateFile(selectedFileNavigationPane);

				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			if (ImGui::Selectable("   Rename"))
				renameFile = file;

			p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Show in Explorer")) {

				ShellExecute(NULL, L"open", std::filesystem::absolute(selectedFileNavigationPane->path).c_str(), NULL, NULL, SW_RESTORE);

				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}
			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}
		ImGui::PopID();

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("_NAVIGATION_PANE_FILE", &file, sizeof(File*));
			ImGui::TextColored(DEFAULT_TEXT_COLOR, file->name.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_NAVIGATION_PANE_FILE"))
			{
				// ok
				IM_ASSERT(payload->DataSize == sizeof(File*));
				File* payload_n = *(File**)payload->Data;
				Core::instance->fileSystem->moveFile(payload_n, file);
				selectedFileMainPane = NULL;
				coloredFileMainPane = NULL;

				if (payload_n == selectedFileNavigationPane) {
					selectedFileNavigationPane = file;
					coloredFileNavigationPane = file;

					Core::instance->fileSystem->currentOpenFile = selectedFileNavigationPane;
				}
				return;
			}

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_MAIN_PANE_FILE"))
			{
				// ok
				IM_ASSERT(payload->DataSize == sizeof(File*));
				File* payload_n = *(File**)payload->Data;
				Core::instance->fileSystem->moveFile(payload_n, file);
				selectedFileMainPane = NULL;
				coloredFileMainPane = NULL;
				return;
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();

		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

		if (nodeOpen && hasSubFolder)
			ImGui::Image((ImTextureID)folderOpened16TextureId, size, uv0, uv1, tint_col, border_col);
		else
			ImGui::Image((ImTextureID)folderClosed16TextureId, size, uv0, uv1, tint_col, border_col);

		ImGui::SameLine();


		ImVec2 pos = ImGui::GetCursorPos();
		if (renameFile == file) {

			char temp[3] = { '#','#', '\0' };
			char str0[32] = "";
			strcat(str0, (char*)file->name.c_str());
			ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
			ImGui::SetKeyboardFocusHere(0);
			ImGui::SetCursorPos(ImVec2(pos.x - 5, pos.y - 2));

			if (ImGui::InputText(temp, str0, IM_ARRAYSIZE(str0), input_text_flags)) {

				if (strlen(str0) != 0)
					Core::instance->fileSystem->rename(file, str0);
			}
		}
		else {

			if (coloredFileNavigationPane == file)
				ImGui::TextColored(TEXT_SELECTED_COLOR, file->name.c_str());
			else
				ImGui::TextColored(DEFAULT_TEXT_COLOR, file->name.c_str());
		}

		for (int i = 0; i < file->subfiles.size(); i++) {



			if (nodeOpen && file->subfiles[i]->type == FileType::folder)
				Menu::createFoldersRecursively(file->subfiles[i]);
		}

		ImGui::Unindent(ImGui::GetStyle().IndentSpacing);
	}

	void Menu::createFilesPanelRightPart(ImVec2 area) {

		anyChildFileHovered = false;

		int maxElementsInRow = (int)((area.x + 26) / 100);

		if (selectedFileNavigationPane == NULL || maxElementsInRow == 0)
			return;

		int frame_padding = 1;
		ImVec2 size = ImVec2(64.0f, 64.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 bg_col = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 5));

		mainPaneFilePopup = false;
		for (int i = 0; i < selectedFileNavigationPane->subfiles.size(); i++) {

			ImGui::BeginGroup();
			{
				ImGui::PushID(i);

				// make it bluish to point out file is selected
				if (coloredFileMainPane == selectedFileNavigationPane->subfiles[i])
					tint_col = ImVec4(0.7f, 0.7f, 1.0f, 1.0f);
				else
					tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

				ImVec2 pos = ImGui::GetCursorPos();
				ImGui::SetCursorPos(ImVec2(pos.x + (100.f - 64.f) / 2, pos.y));

				unsigned int textureId = Menu::getTextureIdForFile(selectedFileNavigationPane->subfiles[i]);
				ImGui::ImageButton((ImTextureID)textureId, size, uv0, uv1, frame_padding, bg_col, tint_col);

				if (ImGui::IsItemHovered())
					anyChildFileHovered = true;

				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) { //  || ImGui::IsItemClicked(ImGuiMouseButton_Right)

					holdedFileMainPane = selectedFileNavigationPane->subfiles[i];
					coloredFileMainPane = holdedFileMainPane;
					mousePosWhenClicked = ImGui::GetMousePos();
				}

				ImGui::SetNextWindowSize(ImVec2(210, 145)); // each item height is 25
				if (ImGui::BeginPopupContextItem("context_menu_item_popup"))
				{
					coloredFileMainPane = selectedFileNavigationPane->subfiles[i];
					selectedFileMainPane = selectedFileNavigationPane->subfiles[i];
					selectedEntity = NULL;
					coloredEntity = NULL;

					mainPaneFilePopup = true;

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

					if (ImGui::Selectable("   Open")) {

						if (selectedFileMainPane->type == FileType::folder) {
							selectedFileNavigationPane = selectedFileMainPane;

							Core::instance->fileSystem->currentOpenFile = selectedFileNavigationPane;
						}
						else
							ShellExecute(NULL, L"open", std::filesystem::absolute(selectedFileMainPane->path).c_str(), NULL, NULL, SW_RESTORE);

						// include all the necessary end codes...
						ImGui::PopStyleColor();
						ImGui::EndPopup();
						ImGui::PopID();
						ImGui::EndGroup();
						return;
					}

					ImVec2 p = ImGui::GetCursorScreenPos();
					ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
					ImGui::Dummy(ImVec2(0, 1));

					if (ImGui::Selectable("   Delete")) {

						Core::instance->fileSystem->deleteFileCompletely(selectedFileMainPane);
						selectedFileMainPane = NULL;
						coloredFileMainPane = NULL;

						// include all the necessary end codes...
						ImGui::PopStyleColor();
						ImGui::EndPopup();
						ImGui::PopID();
						ImGui::EndGroup();
						return;
					}

					if (ImGui::Selectable("   Duplicate")) {

						Core::instance->fileSystem->duplicateFile(selectedFileMainPane);

						// include all the necessary end codes...
						ImGui::PopStyleColor();
						ImGui::EndPopup();
						ImGui::PopID();
						ImGui::EndGroup();
						return;
					}

					if (ImGui::Selectable("   Rename"))
						renameFile = selectedFileNavigationPane->subfiles[i];

					p = ImGui::GetCursorScreenPos();
					ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
					ImGui::Dummy(ImVec2(0, 1));

					if (ImGui::Selectable("   Open Scene")) {

						Core::instance->sceneManager->loadScene(selectedFileMainPane->name);
						Core::instance->sceneManager->saveSceneManagerFile();

						// include all the necessary end codes...
						ImGui::PopStyleColor();
						ImGui::EndPopup();
						ImGui::PopID();
						ImGui::EndGroup();
						return;
					}

					p = ImGui::GetCursorScreenPos();
					ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
					ImGui::Dummy(ImVec2(0, 1));

					if (ImGui::Selectable("   Show in Explorer")) {

						ShellExecute(NULL, L"open", std::filesystem::absolute(selectedFileNavigationPane->path).c_str(), NULL, NULL, SW_RESTORE);

						// include all the necessary end codes...
						ImGui::PopStyleColor();
						ImGui::EndPopup();
						ImGui::PopID();
						ImGui::EndGroup();
						return;
					}
					ImGui::PopStyleColor();
					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("_MAIN_PANE_FILE", &selectedFileNavigationPane->subfiles[i], sizeof(File*));
					ImGui::TextColored(DEFAULT_TEXT_COLOR, selectedFileNavigationPane->subfiles[i]->name.c_str());
					ImGui::EndDragDropSource();
				}

				dragToMainPaneFile = false;
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_MAIN_PANE_FILE"))
					{
						// ok
						IM_ASSERT(payload->DataSize == sizeof(File*));
						File* payload_n = *(File**)payload->Data;
						Core::instance->fileSystem->moveFile(payload_n, selectedFileNavigationPane->subfiles[i]);
						dragToMainPaneFile = true;
						// include all the necessary end codes...
						ImGui::PopID();
						ImGui::EndGroup();
						return;
					}

					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_NAVIGATION_PANE_FILE"))
					{
						IM_ASSERT(payload->DataSize == sizeof(File*));
						File* payload_n = *(File**)payload->Data;

						Core::instance->fileSystem->moveFile(payload_n, selectedFileNavigationPane->subfiles[i]);
						dragToMainPaneFile = true;
						// include all the necessary end codes...
						ImGui::PopID();
						ImGui::EndGroup();
						return;
					}

					ImGui::EndDragDropTarget();
				}
				ImGui::PopID();
				ImGui::PushItemWidth(64.0f);

				ImVec4 textColor = coloredFileMainPane == selectedFileNavigationPane->subfiles[i] ? TEXT_SELECTED_COLOR : DEFAULT_TEXT_COLOR;
				pos = ImGui::GetCursorPos();

				if (renameFile == selectedFileNavigationPane->subfiles[i]) {

					char temp[3] = { '#','#', '\0' };
					char str0[32] = "";
					strcat(str0, (char*)selectedFileNavigationPane->subfiles[i]->name.c_str());
					ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
					ImGui::SetKeyboardFocusHere(0);
					ImVec2 textSize = ImGui::CalcTextSize(str0);
					ImGui::SetCursorPos(ImVec2(pos.x + (100.f - textSize.x) / 2 - 5, pos.y - 2));

					if (ImGui::InputText(temp, str0, IM_ARRAYSIZE(str0), input_text_flags)) {

						if (strlen(str0) != 0)
							Core::instance->fileSystem->rename(selectedFileNavigationPane->subfiles[i], str0);
					}
				}
				else {

					int len = selectedFileNavigationPane->subfiles[i]->name.length();

					if (len > 11) {

						char dots[] = "..\0";
						char fileName[10];
						strcpy(fileName, selectedFileNavigationPane->subfiles[i]->name.substr(0, 7).c_str());
						strcat(fileName, dots);

						ImVec2 textSize = ImGui::CalcTextSize(fileName);
						ImGui::SetCursorPos(ImVec2(pos.x + (100.f - textSize.x) / 2, pos.y));

						ImGui::TextColored(textColor, fileName);
					}
					else {

						ImVec2 textSize = ImGui::CalcTextSize(selectedFileNavigationPane->subfiles[i]->name.c_str());
						ImGui::SetCursorPos(ImVec2(pos.x + (100.f - textSize.x) / 2, pos.y));

						ImGui::TextColored(textColor, selectedFileNavigationPane->subfiles[i]->name.c_str());
					}
				}
				ImGui::EndGroup();
			}
			if ((i + 1) % maxElementsInRow != 0)
				ImGui::SameLine(0);
		}
	}

	void Menu::showCurrentDirectoryText() {

		if (selectedFileNavigationPane == NULL)
			return;

		std::stack<File*>fileStack;
		File* iter = selectedFileNavigationPane;

		if (iter->type != FileType::folder)
			iter = iter->parent;

		while (iter != NULL) { // iter->parent != NULL

			fileStack.push(iter);
			iter = iter->parent;
		}

		while (fileStack.size() > 1) {

			File* popped = fileStack.top();
			fileStack.pop();

			if(popped == holdedFileDirectoryText)
				ImGui::TextColored(TEXT_SELECTED_COLOR, popped->name.c_str());
			else
				ImGui::TextColored(DEFAULT_TEXT_COLOR, popped->name.c_str());

			if (ImGui::IsItemClicked()) {
				holdedFileDirectoryText = popped;
				coloredFileDirectoryText = holdedFileDirectoryText;
				mousePosWhenClicked = ImGui::GetMousePos();

				selectedFileMainPane = NULL;
				coloredFileMainPane = NULL;
			}
			ImGui::SameLine(0, 0);

			ImVec2 uv_min = ImVec2(0.0f, 0.0f);
			ImVec2 uv_max = ImVec2(1.0f, 1.0f);
			ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			ImVec4 border_col = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
			ImGui::Image((ImTextureID)greaterTextureId, ImVec2(16.f, 16.f), uv_min, uv_max, tint_col, border_col);

			ImGui::SameLine(0, 0);
		}
		File* popped = fileStack.top();
		fileStack.pop();

		ImGui::TextColored(WHITE, popped->name.c_str());

		if (ImGui::IsItemClicked()) {
			holdedFileDirectoryText = popped;
			coloredFileDirectoryText = holdedFileDirectoryText;
			mousePosWhenClicked = ImGui::GetMousePos();
		}

		//if (fileSystemControlVars.lastSelectedItem != NULL && fileSystemControlVars.lastSelectedItem->type != FileType::folder) {

		//	ImGui::SameLine();
		//	char line[] = " | \0";
		//	char fileName[32];
		//	strcpy(fileName, line);
		//	strcat(fileName, fileSystemControlVars.lastSelectedItem->name.c_str());
		//	strcat(fileName, fileSystemControlVars.lastSelectedItem->extension.c_str());
		//	ImGui::Text(fileName);
		//}
	}

	void Menu::createInspectorPanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 5));

		ImGui::Begin("Inspector");

		ImGui::PopStyleVar();

		ImGui::Indent(6);

		if (selectedEntity) {

			Menu::showEntityName();

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 5));

			ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.23f, 0.23f, 0.23f, 1.f));

			int index = 2;
			Menu::showTransformComponent(index++);

			if (MeshRenderer* meshRendererComp = selectedEntity->getComponent<MeshRenderer>()) {

				//MaterialFile& material = *meshRendererComp->mat;
				Menu::showMeshRendererComponent(index++, meshRendererComp);
				Menu::showMaterialProperties(index++, meshRendererComp->materialFile);
			}

			if (Terrain* terrainComp = selectedEntity->getComponent<Terrain>()) {

				//MaterialFile& material = *meshRendererComp->mat;
				Menu::showTerrainComponent(index++, terrainComp);
				//Menu::showMaterialProperties(material, index++);
			}

			//if (TerrainGenerator* terrainComp = lastSelectedEntity->getComponent<TerrainGenerator>()) {

			//	MaterialFile& material = *terrainComp->mat;
			//	Menu::showTerrainGeneratorComponent(terrainComp, index++);
			//	//Menu::showMaterialProperties(material, index++);
			//}

			//if (Light* lightComp = lastSelectedEntity->getComponent<Light>())
			//	Menu::showLightComponent(index++);

			//if (Rigidbody* rigidbodyComp = lastSelectedEntity->getComponent<Rigidbody>())
			//	Menu::showRigidbodyComponent(index++);

			//std::vector<BoxCollider*> boxColliderCompList = lastSelectedEntity->getComponents<BoxCollider>();
			//int boxColliderCompIndex = 0;
			//for (auto& comp : boxColliderCompList)
			//	Menu::showBoxColliderComponent(comp, index++, boxColliderCompIndex++);

			//std::vector<SphereCollider*> sphereColliderCompList = lastSelectedEntity->getComponents<SphereCollider>();
			//int sphereColliderCompIndex = 0;
			//for (auto& comp : sphereColliderCompList)
			//	Menu::showSphereColliderComponent(comp, index++, sphereColliderCompIndex++);

			//std::vector<CapsuleCollider*> capsuleColliderCompList = lastSelectedEntity->getComponents<CapsuleCollider>();
			//int capsuleColliderCompIndex = 0;
			//for (auto& comp : capsuleColliderCompList)
			//	Menu::showCapsuleColliderComponent(comp, index++, capsuleColliderCompIndex++);

			//std::vector<MeshCollider*> meshColliderCompList = lastSelectedEntity->getComponents<MeshCollider>();
			//int meshColliderCompIndex = 0;
			//for (auto& comp : meshColliderCompList)
			//	Menu::showMeshColliderComponent(comp, index++, meshColliderCompIndex++);

			if (GameCamera* gameCameraComp = selectedEntity->getComponent<GameCamera>())
				Menu::showGameCameraComponent(gameCameraComp, index++);

			ImGui::PopStyleColor(); // for the separator

			Menu::addComponentButton();
		}

		if (selectedFileMainPane) {

			switch (selectedFileMainPane->type) {

			case FileType::mat: {

				MaterialFile* mat = Core::instance->fileSystem->fileToMatFile[selectedFileMainPane];
				Menu::showMaterialProperties(0, mat);
				break;
			}
			case FileType::png:
				Menu::showTextureProperties();
				break;
			}
		}
		//if (fileSystemControlVars.lastSelectedItemID != -1 && editor->fileSystem->files[fileSystemControlVars.lastSelectedItemID].type == FileType::physicmaterial)
		//	Menu::showPhysicMaterialProperties(editor->fileSystem->getPhysicMaterialFile(fileSystemControlVars.lastSelectedItemID));

		ImGui::Unindent(6);

		inspectorHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
			ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows);

		ImGui::End();
	}

	void Menu::addComponentButton() {

		ImVec2 pos = ImGui::GetCursorPos();
		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 5));

		ImGui::Indent((width - 120) / 2);
		if (ImGui::Button("Add Component", ImVec2(120, 23))) {

			pos = ImVec2(ImGui::GetCursorScreenPos().x - 25, ImGui::GetCursorScreenPos().y);
			ImGui::SetNextWindowPos(pos);
			ImGui::OpenPopup("add_component_popup");

		}
		ImGui::Unindent((width - 120) / 2);

		ImGui::SetNextWindowSize(ImVec2(180, 260));

		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.f));
		if (ImGui::BeginPopupContextItem("add_component_popup"))
		{
			if (ImGui::Selectable("   Animation")) {

			}
			//ImGui::Separator();

			//if (ImGui::Selectable("   Animator")) { // bir gun buraya gelinirse o zaman buyuk bir is yapilmis demektir ;)

			//}

			ImGui::Separator();

			if (ImGui::Selectable("   Game Camera")) {

				GameCamera* gameCamComp = selectedEntity->getComponent<GameCamera>();

				if (!gameCamComp) {

					gameCamComp = selectedEntity->addComponent<GameCamera>();
					Core::instance->sceneManager->currentScene->primaryCamera = gameCamComp;
					gameCamComp->init(selectedEntity->transform, gameRegion.x, gameRegion.y);
				}
				
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Collider (Box)")) {

				//BoxCollider* boxColliderComp = lastSelectedEntity->addComponent<BoxCollider>();
				//boxColliderComp->init(editor);
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Collider (Capsule)")) {

				//CapsuleCollider* capsuleColliderComp = lastSelectedEntity->addComponent<CapsuleCollider>();
				//capsuleColliderComp->init(editor);
				// 
				//capsuleColliderComp->pmat = &editor->fileSystem.physicmaterials["Default"];

				//float max = lastSelectedEntity->transform->globalScale.x > lastSelectedEntity->transform->globalScale.y ?
				//	lastSelectedEntity->transform->globalScale.x : lastSelectedEntity->transform->globalScale.y;
				//max = max > lastSelectedEntity->transform->globalScale.z ? max : lastSelectedEntity->transform->globalScale.z;

				//float halfHeight = lastSelectedEntity->transform->globalScale * capsuleColliderComp->height / 2.f;
				//capsuleColliderComp->shape = editor->physics.gPhysics->createShape(PxCapsuleGeometry(halfHeight, ), *sphereColliderComp->pmat->pxmat);
				//sphereColliderComp->shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
				//glm::vec3 center = sphereColliderComp->transform->globalScale * sphereColliderComp->center;
				//sphereColliderComp->shape->setLocalPose(PxTransform(center.x, center.y, center.z));

				//if (Rigidbody* rb = lastSelectedEntity->getComponent<Rigidbody>())
				//	rb->body->attachShape(*sphereColliderComp->shape);
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Collider (Mesh)")) {

				//if (MeshRenderer* mr = lastSelectedEntity->getComponent<MeshRenderer>()) {

				//	MeshCollider* meshColliderComp = lastSelectedEntity->addComponent<MeshCollider>();
				//	Rigidbody* rb = lastSelectedEntity->getComponent<Rigidbody>();
				//	meshColliderComp->init(editor, rb, mr, true);
				//}
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Collider (Sphere)")) {

				//SphereCollider* sphereColliderComp = lastSelectedEntity->addComponent<SphereCollider>();
				//sphereColliderComp->init(editor);
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Light")) {

				//if (Light* lightComp = lastSelectedEntity->addComponent<Light>()) {

				//	Transform* lighTransform = lastSelectedEntity->transform;
				//	editor->scene->pointLightTransforms.push_back(lighTransform);
				//	editor->scene->recompileAllMaterials();
				//}
				//else
				//	statusMessage = "There is existing component in the same type!";
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Mesh Renderer")) {

				// bunlari daha duzenli hale getirebilirsin

				if (MeshRenderer* meshRendererComp = selectedEntity->addComponent<MeshRenderer>()) {

					meshRendererComp->meshFile = NULL;
					meshRendererComp->materialFile = NULL;// Core::instance->fileSystem->pbrMaterialNoTexture;
				}
				//else
				//	statusMessage = "There is existing component in the same type!";
			}
			ImGui::Separator();

			if (ImGui::Selectable("   Rigidbody")) {

				//if (Rigidbody* rigidbodyComp = lastSelectedEntity->addComponent<Rigidbody>()) {

				//	glm::quat myquaternion = glm::quat(lastSelectedEntity->transform->globalRotation);
				//	PxTransform tm(PxVec3(lastSelectedEntity->transform->globalPosition.x, lastSelectedEntity->transform->globalPosition.y,
				//		lastSelectedEntity->transform->globalPosition.z),
				//		PxQuat(myquaternion.x, myquaternion.y, myquaternion.z, myquaternion.w));

				//	rigidbodyComp->body = editor->physics->gPhysics->createRigidDynamic(tm);
				//	rigidbodyComp->body->setMass(1.f);
				//	editor->physics->gScene->addActor(*rigidbodyComp->body);

				//	for (auto& comp : lastSelectedEntity->getComponents<Collider>())
				//		rigidbodyComp->body->attachShape(*comp->shape);
				//}
				//else
				//	statusMessage = "There is existing component in the same type!";

			}
			ImGui::Separator();

			if (ImGui::Selectable("   Terrain")) {

				//if (TerrainGenerator* terrainComp = lastSelectedEntity->addComponent<TerrainGenerator>())
				//	terrainComp->init(&editor->fileSystem->materials["Default"]);

				if (Terrain* terrainComp = selectedEntity->addComponent<Terrain>()) {
					terrainComp->init();
				}
			}

			//if (ImGui::Selectable("   Script")) {

			//}

			ImGui::EndPopup();
		}
		ImGui::PopStyleColor();
	}

	void Menu::showEntityName() {

		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

		//ImGui::Image((ImTextureID)0, size, uv0, uv1, tint_col, border_col); //editorIcons.gameObjectTextureID
		//ImGui::SameLine();

		static bool active = true;
		ImGui::Checkbox("##0", &active);
		ImGui::SameLine();

		//int len = strlen(editor->scene->entities[lastSelectedEntityID].name);
		//char* str0 = new char[len + 1];
		//strcpy(str0, editor->scene->entities[lastSelectedEntityID].name);
		//str0[len] = '\0';
		//ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
		//ImVec2 textSize = ImGui::CalcTextSize(str0);

		//if (ImGui::InputText("##9", str0, IM_ARRAYSIZE(str0), input_text_flags)) {

		//	if (strlen(str0) != 0)
		//		editor->scene->renameEntity(editor->scene->entities[lastSelectedEntityID].transform->id, str0);
		//	renameEntityID = -1;
		//}
		//delete[] str0;
		//ImGui::Separator();

		char name[32];
		strcpy(name, selectedEntity->name.c_str());
		ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
		if (ImGui::InputText("##1", name, IM_ARRAYSIZE(name), input_text_flags)) {
			if (strlen(name) != 0)
				selectedEntity->rename(name);
			renameEntity = NULL;
		}

		ImGui::Separator();
	}

	void Menu::showTransformComponent(int index) {

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0' + index;
		str[2] = indexChar;
		bool treeNodeOpen = ImGui::TreeNode(str);

		int frame_padding = 1;
		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
		ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

		ImGui::SameLine(25);
		ImGui::Image((ImTextureID)transformTextureId, size, uv0, uv1, tint_col, border_col);
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, " Transform");

		ImGui::SameLine();
		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(width - 20, pos.y));

		ImGui::ImageButton((ImTextureID)contextMenuTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col);
		//if (ImGui::ImageButton((ImTextureID)0, size, uv0, uv1, frame_padding, bg_col, tint_col)) //editorIcons.contextMenuTextureID
			//ImGui::OpenPopup("context_menu_popup");

		//Menu::contextMenuPopup(ComponentType::Transform, 0);

		if (treeNodeOpen) {

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Position  X"); ImGui::SameLine(); ImGui::PushItemWidth((width - 150) / 3);

			if (ImGui::DragFloat("##0", &selectedEntity->transform->localPosition.x, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Y"); ImGui::SameLine(); ImGui::PushItemWidth((width - 150) / 3);

			if (ImGui::DragFloat("##1", &selectedEntity->transform->localPosition.y, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Z"); ImGui::SameLine(); ImGui::PushItemWidth((width - 150) / 3);

			if (ImGui::DragFloat("##2", &selectedEntity->transform->localPosition.z, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Rotation  X"); ImGui::SameLine(); ImGui::PushItemWidth((width - 150) / 3);

			if (ImGui::DragFloat("##3", &selectedEntity->transform->localRotation.x, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Y"); ImGui::SameLine(); ImGui::PushItemWidth((width - 150) / 3);

			if (ImGui::DragFloat("##4", &selectedEntity->transform->localRotation.y, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Z"); ImGui::SameLine(); ImGui::PushItemWidth((width - 150) / 3);

			if (ImGui::DragFloat("##5", &selectedEntity->transform->localRotation.z, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Scale     X"); ImGui::SameLine(); ImGui::PushItemWidth((width - 150) / 3);

			if (ImGui::DragFloat("##6", &selectedEntity->transform->localScale.x, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Y"); ImGui::SameLine(); ImGui::PushItemWidth((width - 150) / 3);

			if (ImGui::DragFloat("##7", &selectedEntity->transform->localScale.y, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Z"); ImGui::SameLine(); ImGui::PushItemWidth((width - 150) / 3);

			if (ImGui::DragFloat("##8", &selectedEntity->transform->localScale.z, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::TreePop();
		}

		ImGui::Separator();
	}

	void Menu::showMeshRendererComponent(int index, MeshRenderer* comp) {

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0' + index;
		str[2] = indexChar;
		bool treeNodeOpen = ImGui::TreeNode(str);

		int frame_padding = 1;
		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
		ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

		ImGui::SameLine(25);
		ImGui::Image((ImTextureID)meshRendererTextureId, size, uv0, uv1, tint_col, border_col);
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, "  Mesh Renderer");

		ImGui::SameLine();
		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(width - 20, pos.y));

		if (ImGui::ImageButton((ImTextureID)contextMenuTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col))
			ImGui::OpenPopup("context_menu_popup");

		Menu::contextMenuPopup(comp, 0);

		if (treeNodeOpen) {

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));

			std::vector<File*>& meshFiles = Core::instance->fileSystem->meshFiles;
			std::vector<File*>& matFiles = Core::instance->fileSystem->matFiles;
			int size_meshes = meshFiles.size() + 1;
			int size_mats = matFiles.size() + 1;

			const char** meshNames = new const char* [size_meshes];
			const char** matNames = new const char* [size_mats];
			meshNames[0] = "Empty";
			matNames[0] = "Empty";

			int meshIndex;
			int matIndex;

			if (comp->meshFile == NULL)
				meshIndex = 0;

			if (comp->materialFile == NULL)
				matIndex = 0;

			if (comp->meshFile) {
				int i = 1;
				File* file = Core::instance->fileSystem->meshFileToFile[comp->meshFile];
				for (auto it : meshFiles) {
					if (it == file)
						meshIndex = i;
					meshNames[i] = it->name.c_str();
					i++;
				}
			}
			else {
				int i = 1;
				for (auto it : meshFiles) {
					meshNames[i] = it->name.c_str();
					i++;
				}
			}

			if (comp->materialFile) {
				int i = 1;
				File* file = Core::instance->fileSystem->matFileToFile[comp->materialFile];
				for (auto it : matFiles) {
					if (it == file)
						matIndex = i;
					matNames[i] = it->name.c_str();
					i++;
				}
			}
			else {
				int i = 1;
				for (auto it : matFiles) {
					matNames[i] = it->name.c_str();
					i++;
				}
			}

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Mesh        ");
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
			ImGui::SetNextItemWidth(width - 115);

			if (ImGui::Combo("##0", &meshIndex, &meshNames[0], size_meshes)) {

				if (meshIndex == 0)
					comp->releaseMeshFile();
				else {
					File* file = meshFiles[meshIndex - 1];
					MeshFile* meshFile = Core::instance->fileSystem->fileToMeshFile[file];
					comp->setMeshFile(file, meshFile);
				}
			}

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Material    ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(width - 115);

			if (ImGui::Combo("##1", &matIndex, matNames, size_mats)) {

				if (matIndex == 0)
					comp->releaseMatFile();
				else {
					File* file = matFiles[matIndex - 1];
					MaterialFile* matFile = Core::instance->fileSystem->fileToMatFile[file];
					comp->setMatFile(file, matFile);
				}
			}

			delete[] meshNames;
			delete[] matNames;

			ImGui::PopStyleColor();
			ImGui::TreePop();
		}

		ImGui::Separator();
	}

	void Menu::showTerrainComponent(int index, Terrain* comp) {

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0' + index;
		str[2] = indexChar;
		bool treeNodeOpen = ImGui::TreeNode(str);

		int frame_padding = 1;
		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
		ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

		ImGui::SameLine(25);
		ImGui::Image((ImTextureID)meshRendererTextureId, size, uv0, uv1, tint_col, border_col);
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, "  Terrain");

		ImGui::SameLine();
		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(width - 20, pos.y));

		if (ImGui::ImageButton((ImTextureID)contextMenuTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col))
			ImGui::OpenPopup("context_menu_popup");

		Menu::contextMenuPopup(comp, 0);

		if (treeNodeOpen) {

			ImGui::Image((ImTextureID)comp->elevationMapTexture, ImVec2(128, 128), uv0, uv1, tint_col, border_col);

			ImGui::TreePop();
		}

		ImGui::Separator();
	}

	void Menu::showGameCameraComponent(GameCamera* camComp, int index) {

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0' + index;
		str[2] = indexChar;
		bool treeNodeOpen = ImGui::TreeNode(str);

		int frame_padding = 1;
		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
		ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

		ImGui::SameLine(25);
		ImGui::Image((ImTextureID)cameraTextureId, size, uv0, uv1, tint_col, border_col);
		ImGui::SameLine();
		ImGui::Text("  Game Camera");

		ImGui::SameLine();
		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(width - 20, pos.y));

		if (ImGui::ImageButton((ImTextureID)contextMenuTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col))
			ImGui::OpenPopup("context_menu_popup");

		Menu::contextMenuPopup(camComp, 0);

		//if (EditorGUI::contextMenuPopup(ComponentType::GameCamera, 0)) {

		//	ImGui::TreePop();
		//	return;
		//}

		if (treeNodeOpen) {

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));

			const char** projectionTypes = new const char* [2];
			projectionTypes[0] = "Perspective";
			projectionTypes[1] = "Orthographic";

			const char** fovAxis = new const char* [2];
			fovAxis[0] = "Vertical";
			fovAxis[1] = "Horizontal";

			ImGui::Text("Projection    ");
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
			ImGui::SetNextItemWidth(width - 130);

			ImGui::Combo("##0", &camComp->projectionType, projectionTypes, 2);
			/*if (ImGui::Combo("##0", &camComp->projectionType, projectionTypes, 2))
				camComp->createEditorGizmos(true);*/

			ImGui::Text("FOV Axis      ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(width - 130);
			int oldFovAxis = camComp->fovAxis;
			if (ImGui::Combo("##1", &camComp->fovAxis, fovAxis, 2))
				camComp->changeFovAxis(oldFovAxis);

			ImGui::Text("FOV           ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(width - 130);
			if (ImGui::DragFloat("##2", &camComp->fov, 1.f, 1.f, 179.f, "%.1f"))
				camComp->updateEditorGizmos();

			ImGui::Text("Near          ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(width - 130);
			if (ImGui::DragFloat("##3", &camComp->nearClip, 0.01f, 0.f, 10.f, "%.2f"))
				camComp->updateEditorGizmos();

			ImGui::Text("Far           ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(width - 130);
			if (ImGui::DragFloat("##4", &camComp->farClip, 1.f, 100.f, 10000.f, "%.2f"))
				camComp->updateEditorGizmos();

			delete[] projectionTypes;
			delete[] fovAxis;

			ImGui::PopStyleColor();
			ImGui::TreePop();
		}

		ImGui::Separator();
	}

	void Menu::showMaterialProperties(int index, MaterialFile* mat) {

		if (!mat) return;

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0' + index;
		str[2] = indexChar;
		bool treeNodeOpen = ImGui::TreeNode(str);

		int frame_padding = 2;
		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
		ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

		ImGui::SameLine(28);
		ImGui::Image((ImTextureID)mat->fileTextureId, size, uv0, uv1, tint_col, border_col); // editorIcons.materialSmallTextureID
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, "  Material");

		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.f));

		if (treeNodeOpen) {

		//	if (mat != Core::instance->fileSystem->pbrMaterialNoTexture) {
				ImGui::TextColored(DEFAULT_TEXT_COLOR, "Vert Shader");
				ImGui::SameLine(115); ImGui::SetNextItemWidth(width - 115);
				int shaderType = mat->shaderTypeId;
				const char** shaderNames = new const char* [1];
				shaderNames[0] = "PBR\0";

				if (ImGui::Combo("##0", &shaderType, shaderNames, 1)) {

					mat->shaderTypeId = shaderType;

					/*if (shaderType == 0) {
						File* file = Core::instance->fileSystem->matFileToFile[mat];
						mat->loadPBRShaderProgramWithoutTexture(file);
					}

					if (shaderType == 1) {
						File* file = Core::instance->fileSystem->matFileToFile[mat];
						mat->loadPBRShaderProgramWithTexture(file);
					}*/
				}

				delete[] shaderNames;

			

			ImVec2 pos = ImGui::GetCursorPos();
			//for (int i = 0; i < mat->textureFiles.size(); i++) {

			//	ImGui::PushID(i);

			//	unsigned int textureId = mat->textureFiles[i] ? mat->textureFiles[i]->textureId : 0;
			//	if (ImGui::ImageButton((ImTextureID)textureId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {
			//		//ImGui::OpenPopup("texture_menu_popup");
			//		//popupFlag = true;
			//	}

			//	if (ImGui::BeginDragDropTarget())
			//	{
			//		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_MAIN_PANE_FILE"))
			//		{
			//			IM_ASSERT(payload->DataSize == sizeof(File*));
			//			File* payload_n = *(File**)payload->Data;
			//			mat->insertTexture(i, payload_n, core);
			//			mat->loadPBRShaderProgramWithTexture(payload_n->path, core);
			//		}
			//		ImGui::EndDragDropTarget();
			//	}

			//	//EditorGUI::textureMenuPopup(material, i, popupFlag);

			//	ImGui::PopID();
			//	ImGui::SameLine();
			//	pos = ImGui::GetCursorPos(); ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));
			//	std::string str = "TextureUnit " + std::to_string(i + 1);
			//	ImGui::Text(&str[0]);
			//	//ImGui::Text(fragShaders[shaderPaths[index]].sampler2DNames[i].c_str());
			//}

			// bu kisimlar cok daha moduler yapilabilir.
			// index mapten cekilir.
			// stringler mapten cekilir vsvs...
				if (shaderType == 0) {

					int counter = 0;
					for (int i = 0; i < 5; i++) {

						ImGui::PushID(i);

						if (std::count(mat->activeTextureIndices.begin(), mat->activeTextureIndices.end(), i)) {

							if (ImGui::ImageButton((ImTextureID)mat->textureFiles[counter]->textureId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {
								//ImGui::OpenPopup("texture_menu_popup");
								//popupFlag = true;
							}

							if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
								mat->releaseTexFile(counter);

								ImGui::PopID();
								ImGui::TreePop();
								ImGui::PopStyleColor();
								return;
							}

							counter++;
						}
						else {

							if (ImGui::ImageButton((ImTextureID)0, size, uv0, uv1, frame_padding, bg_col, tint_col)) {
								//ImGui::OpenPopup("texture_menu_popup");
								//popupFlag = true;
							}
						}

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_MAIN_PANE_FILE"))
							{
								IM_ASSERT(payload->DataSize == sizeof(File*));
								File* payload_n = *(File**)payload->Data;
								//File* file = Core::instance->fileSystem->matFileToFile[mat];
								mat->insertTexture(i, payload_n); // , file
							}
							ImGui::EndDragDropTarget();
						}

						//EditorGUI::textureMenuPopup(material, i, popupFlag);

						ImGui::PopID();
						ImGui::SameLine();
						pos = ImGui::GetCursorPos(); ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));
						std::string str = "TextureUnit " + std::to_string(i + 1); // bunun yerine albedo filan yazilabilir.
						ImGui::TextColored(DEFAULT_TEXT_COLOR, &str[0]);
						//ImGui::Text(fragShaders[shaderPaths[index]].sampler2DNames[i].c_str());
					}
				}
			//}

			// index devam ettirilerek moduler bir component yapisi olusturulabilir.
			// olmasi gerektigi gibi calisiyor fazla kasma, zorlama...

			ImGui::TreePop();
		}
		ImGui::PopStyleColor();
		ImGui::Separator();
	}

	bool Menu::contextMenuPopup(Component* component, int index) {

		ImGui::SetNextWindowSize(ImVec2(180, 95));

		if (ImGui::BeginPopup("context_menu_popup"))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

			if (ImGui::Selectable("   Copy Values")) {

			}

			if (ImGui::Selectable("   Paste Values")) {

			}

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Reset")) {

			}

			if (ImGui::Selectable("   Remove")) {

				//selectedEntity->removeComponent<Component>();
				deleteBuffer = component;

				ImGui::PopStyleColor();
				ImGui::EndPopup();

				return true;
			}

			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}

		return false;
	}

	void Menu::showTextureProperties() {

		TextureFile* tex = Core::instance->fileSystem->fileToTexFile[selectedFileMainPane];

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);
		bool treeNodeOpen = ImGui::TreeNode("##0");

		int frame_padding = 2;
		ImVec2 size16 = ImVec2(16.0f, 16.0f);
		ImVec2 size256 = ImVec2(256.0f, 256.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
		ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);


		ImGui::SameLine(28);
		ImGui::Image((ImTextureID)0, size16, uv0, uv1, tint_col, border_col); // editorIcons.materialSmallTextureID
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, "  Texture");

		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.f));

		if (treeNodeOpen) {

			ImGui::Image((ImTextureID)tex->textureId, size256, uv0, uv1, tint_col, border_col);
			ImGui::TreePop();
		}
		ImGui::PopStyleColor();
		ImGui::Separator();
	}

	unsigned int Menu::getTextureIdForFile(File* file) {

		switch (file->type) {

		case FileType::folder: {
			return folder64TextureId;
		}
		case FileType::png: {
			return file->textureID;
		}
		case FileType::mat: {
			return file->textureID;
		}
		case FileType::obj: {
			return file->textureID;
		}
		case FileType::scene: {
			return sceneFileTextureId;
		}
		}
		return 0;
	}

	void Menu::initDefaultIcons() {

		unsigned int width, height;
		std::vector<unsigned char> image;
		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/folder_64.png");
		Core::instance->glewContext->generateTexture(folder64TextureId, width, height, &image[0]);
		image.clear();
		
		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/folder_closed_16.png");
		Core::instance->glewContext->generateTexture(folderClosed16TextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/greater_16.png");
		Core::instance->glewContext->generateTexture(greaterTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/pause_16.png");
		Core::instance->glewContext->generateTexture(pauseTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/stop_16.png");
		Core::instance->glewContext->generateTexture(stopTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/start_16.png");
		Core::instance->glewContext->generateTexture(startTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/scene.png");
		Core::instance->glewContext->generateTexture(sceneFileTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/mesh_renderer.png");
		Core::instance->glewContext->generateTexture(meshRendererTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/mesh_collider.png");
		Core::instance->glewContext->generateTexture(meshColliderTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/context_menu.png");
		Core::instance->glewContext->generateTexture(contextMenuTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/transform_16.png");
		Core::instance->glewContext->generateTexture(transformTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/camera.png");
		Core::instance->glewContext->generateTexture(cameraTextureId, width, height, &image[0]);
		image.clear();

		TextureFile::decodeTextureFile(width, height, image, "C:/Projects/Fury/Editor/resource/icons/folder_opened_16.png");
		Core::instance->glewContext->generateTexture(folderOpened16TextureId, width, height, &image[0]);
	}

	//bool EditorGUI::contextMenuPopup(ComponentType type, int index) {

	//	ImGui::SetNextWindowSize(ImVec2(180, 95));

	//	if (ImGui::BeginPopup("context_menu_popup"))
	//	{
	//		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

	//		if (ImGui::Selectable("   Copy Values")) {

	//		}

	//		if (ImGui::Selectable("   Paste Values")) {

	//		}

	//		ImVec2 p = ImGui::GetCursorScreenPos();
	//		ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
	//		ImGui::Dummy(ImVec2(0, 1));

	//		if (ImGui::Selectable("   Reset")) {

	//		}

	//		if (ImGui::Selectable("   Remove")) {

	//			switch (type) {
	//			case ComponentType::Animation: {
	//				break;
	//			}
	//			case ComponentType::Animator: {
	//				break;
	//			}
	//			case ComponentType::BoxCollider: {

	//				BoxCollider* boxColliderComp = lastSelectedEntity->getComponents<BoxCollider>()[index];
	//				boxColliderComp->pmat->removeReference(boxColliderComp);
	//				PxShape* shape = boxColliderComp->shape;
	//				lastSelectedEntity->removeComponent(boxColliderComp);

	//				if (Rigidbody* rb = lastSelectedEntity->getComponent<Rigidbody>())
	//					rb->body->detachShape(*shape);
	//				else
	//					shape->release();
	//				// I think it should be released, because it is probably not shared.

	//				break;
	//			}
	//			case ComponentType::CapsuleCollider: {

	//				CapsuleCollider* capsuleColliderComp = lastSelectedEntity->getComponents<CapsuleCollider>()[index];
	//				capsuleColliderComp->pmat->removeReference(capsuleColliderComp);
	//				PxShape* shape = capsuleColliderComp->shape;
	//				lastSelectedEntity->removeComponent(capsuleColliderComp);

	//				if (Rigidbody* rb = lastSelectedEntity->getComponent<Rigidbody>())
	//					rb->body->detachShape(*shape);
	//				else
	//					shape->release();
	//				// I think it should be released, because it is probably not shared.

	//				break;
	//			}
	//			case ComponentType::Light: {

	//				LightType type = lastSelectedEntity->getComponent<Light>()->lightType;
	//				lastSelectedEntity->removeComponent<Light>();

	//				if (type == LightType::PointLight) {

	//					for (auto it = editor->scene->pointLightTransforms.begin(); it < editor->scene->pointLightTransforms.end(); it++) {

	//						if (*it == lastSelectedEntity->transform) {
	//							editor->scene->pointLightTransforms.erase(it);
	//							break;
	//						}
	//					}
	//				}
	//				else {

	//					for (auto it = editor->scene->dirLightTransforms.begin(); it < editor->scene->dirLightTransforms.end(); it++) {

	//						if (*it == lastSelectedEntity->transform) {
	//							editor->scene->dirLightTransforms.erase(it);
	//							break;
	//						}
	//					}
	//				}
	//				editor->scene->recompileAllMaterials();

	//				break;
	//			}
	//			case ComponentType::MeshCollider: {

	//				MeshCollider* meshColliderComp = lastSelectedEntity->getComponents<MeshCollider>()[index];
	//				meshColliderComp->pmat->removeReference(meshColliderComp);
	//				PxShape* shape = meshColliderComp->shape;
	//				lastSelectedEntity->removeComponent(meshColliderComp);

	//				if (Rigidbody* rb = lastSelectedEntity->getComponent<Rigidbody>())
	//					rb->body->detachShape(*shape);
	//				else
	//					shape->release();
	//				// I think it should be released, because it is probably not shared.

	//				break;
	//			}
	//			case ComponentType::MeshRenderer: {

	//				if (!lastSelectedEntity->getComponent<MeshCollider>()) {

	//					MeshRenderer* meshRenderer = lastSelectedEntity->getComponent<MeshRenderer>();
	//					meshRenderer->mat->removeReference(meshRenderer);
	//					lastSelectedEntity->removeComponent<MeshRenderer>();
	//				}
	//				break;
	//			}
	//			case ComponentType::Script: {
	//				break;
	//			}
	//			case ComponentType::SphereCollider: {

	//				SphereCollider* sphereColliderComp = lastSelectedEntity->getComponents<SphereCollider>()[index];
	//				sphereColliderComp->pmat->removeReference(sphereColliderComp);
	//				PxShape* shape = sphereColliderComp->shape;
	//				lastSelectedEntity->removeComponent(sphereColliderComp);

	//				if (Rigidbody* rb = lastSelectedEntity->getComponent<Rigidbody>())
	//					rb->body->detachShape(*shape);
	//				else
	//					shape->release();
	//				// I think it should be released, because it is probably not shared.

	//				break;
	//			}
	//			case ComponentType::Rigidbody: {
	//				Rigidbody* rb = lastSelectedEntity->getComponent<Rigidbody>();

	//				//for (auto& col : lastSelectedEntity->getComponents<Collider>()) WTF
	//					//rb->body->detachShape(*col->shape);

	//				editor->physics->gScene->removeActor(*rb->body);
	//				lastSelectedEntity->removeComponent<Rigidbody>();
	//				break;
	//			}
	//			case ComponentType::Terrain: {

	//				break;
	//			}
	//			}

	//			ImGui::PopStyleColor();
	//			ImGui::EndPopup();

	//			return true;
	//		}

	//		ImGui::PopStyleColor();
	//		ImGui::EndPopup();
	//	}

	//	return false;
	//}
}