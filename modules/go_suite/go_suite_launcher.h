// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "scene/gui/box_container.h"

class Button;

class GoSuiteLauncher : public VBoxContainer {
	GDCLASS(GoSuiteLauncher, VBoxContainer);

	Button *git_button = nullptr;
	Button *agentics_button = nullptr;

	void _on_git_toggled(bool p_pressed);
	void _on_agentics_toggled(bool p_pressed);

protected:
	static void _bind_methods();

public:
	GoSuiteLauncher();
};
