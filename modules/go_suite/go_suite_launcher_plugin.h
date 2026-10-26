// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "editor/plugins/editor_plugin.h"

class EditorDock;

class GoSuiteLauncherPlugin : public EditorPlugin {
	GDCLASS(GoSuiteLauncherPlugin, EditorPlugin);

	EditorDock *dock = nullptr;

protected:
	void _notification(int p_what);

public:
	GoSuiteLauncherPlugin();
};
