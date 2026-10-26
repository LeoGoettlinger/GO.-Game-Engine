// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "editor/docks/editor_dock.h"

class GoSuiteRegistry {
	static EditorDock *git_dock;
	static EditorDock *agentics_dock;

public:
	static void set_git_dock(EditorDock *p_dock) { git_dock = p_dock; }
	static EditorDock *get_git_dock() { return git_dock; }
	static void set_agentics_dock(EditorDock *p_dock) { agentics_dock = p_dock; }
	static EditorDock *get_agentics_dock() { return agentics_dock; }
};
