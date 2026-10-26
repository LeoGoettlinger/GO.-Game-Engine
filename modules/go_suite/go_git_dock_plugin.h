// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "editor/plugins/editor_plugin.h"

class EditorDock;
class GoGitDock;

class GoGitDockPlugin : public EditorPlugin {
	GDCLASS(GoGitDockPlugin, EditorPlugin);

	EditorDock *dock = nullptr;
	GoGitDock *content = nullptr;

protected:
	void _notification(int p_what);

public:
	GoGitDock *get_content() const { return content; }
	GoGitDockPlugin();
};
