// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "register_types.h"

#ifdef TOOLS_ENABLED
#include "core/object/class_db.h"
#include "editor/plugins/editor_plugin.h"

#include "go_agentics_dock_plugin.h"
#include "go_git_dock_plugin.h"
#include "go_remote_assistant.h"
#include "go_suite_launcher_plugin.h"
#endif // TOOLS_ENABLED

void initialize_go_suite_module(ModuleInitializationLevel p_level) {
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		// Register before construction: its constructor connects inherited signals.
		GDREGISTER_INTERNAL_CLASS(GoRemoteAssistant);
		EditorPlugins::add_by_type<GoGitDockPlugin>();
		EditorPlugins::add_by_type<GoAgenticsDockPlugin>();
		EditorPlugins::add_by_type<GoSuiteLauncherPlugin>();
	}
#endif // TOOLS_ENABLED
}

void uninitialize_go_suite_module(ModuleInitializationLevel p_level) {
}
