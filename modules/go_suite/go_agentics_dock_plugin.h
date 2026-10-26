// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "editor/plugins/editor_plugin.h"

class EditorDock;
class GoAgenticsDock;

class GoAgenticsDockPlugin : public EditorPlugin {
	GDCLASS(GoAgenticsDockPlugin, EditorPlugin);

	EditorDock *dock = nullptr;
	GoAgenticsDock *content = nullptr;

protected:
	void _notification(int p_what);

public:
	GoAgenticsDock *get_content() const { return content; }
	GoAgenticsDockPlugin();
};
