// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_agentics_dock_plugin.h"

#include "go_agentics_dock.h"
#include "go_suite_icons.h"
#include "go_suite_registry.h"

#include "editor/docks/editor_dock.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/themes/editor_scale.h"
#include "core/string/ustring.h"

void GoAgenticsDockPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (dock != nullptr) {
				break;
			}
			dock = memnew(EditorDock);
			dock->set_name("GO. Code Agentics");
			dock->set_title(TTR("Code Agentics"));
			dock->set_layout_key("go_code_agentics");
			dock->set_dock_icon(go_suite::agentics_dock_icon());
			dock->set_default_slot(EditorDock::DOCK_SLOT_RIGHT_UR);
			dock->set_custom_minimum_size(Size2(320, 0) * EDSCALE);

			content = memnew(GoAgenticsDock);
			dock->add_child(content);
			EditorDockManager *manager = EditorDockManager::get_singleton();
			if (manager != nullptr) {
				manager->add_dock(dock);
				GoSuiteRegistry::set_agentics_dock(dock);
			} else {
				dock->queue_free();
				dock = nullptr;
				content = nullptr;
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			GoSuiteRegistry::set_agentics_dock(nullptr);
			EditorDockManager *manager = EditorDockManager::get_singleton();
			if (dock != nullptr && manager != nullptr) {
				manager->remove_dock(dock);
			}
			if (dock != nullptr) {
				dock->queue_free();
				dock = nullptr;
			}
			content = nullptr;
		} break;
	}
}

GoAgenticsDockPlugin::GoAgenticsDockPlugin() {
}
