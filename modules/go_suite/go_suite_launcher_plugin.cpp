// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_suite_launcher_plugin.h"

#include "go_suite_icons.h"
#include "go_suite_launcher.h"

#include "editor/docks/editor_dock.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/themes/editor_scale.h"
#include "core/string/ustring.h"

void GoSuiteLauncherPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (dock != nullptr) {
				break;
			}
			dock = memnew(EditorDock);
			dock->set_name("GO. Suites");
			dock->set_title(TTR("GO. Suites"));
			dock->set_layout_key("go_suites_launcher");
			dock->set_dock_icon(go_suite::launcher_icon());
			dock->set_default_slot(EditorDock::DOCK_SLOT_RIGHT_UL);
			dock->set_custom_minimum_size(Size2(180, 0) * EDSCALE);
			dock->add_child(memnew(GoSuiteLauncher));

			EditorDockManager *manager = EditorDockManager::get_singleton();
			if (manager != nullptr) {
				manager->add_dock(dock);
			} else {
				dock->queue_free();
				dock = nullptr;
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			EditorDockManager *manager = EditorDockManager::get_singleton();
			if (dock != nullptr && manager != nullptr) {
				manager->remove_dock(dock);
			}
			if (dock != nullptr) {
				dock->queue_free();
				dock = nullptr;
			}
		} break;
	}
}

GoSuiteLauncherPlugin::GoSuiteLauncherPlugin() {
}
