// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "go_git_controller.h"

#include "scene/gui/dialogs.h"

class Button;
class Label;
class TextEdit;

class GoLfsDialog : public AcceptDialog {
	GDCLASS(GoLfsDialog, AcceptDialog);

	go_suite::GoGitController *controller = nullptr;
	Label *state_label = nullptr;
	TextEdit *output = nullptr;
	Button *check_button = nullptr;
	Button *install_button = nullptr;
	Button *init_button = nullptr;
	Button *rules_button = nullptr;
	Button *track_button = nullptr;
	Button *pull_button = nullptr;
	Button *push_button = nullptr;
	Button *quota_button = nullptr;

	void _append(const String &p_text);
	void _refresh_state();
	void _on_check_pressed();
	void _on_install_pressed();
	void _on_init_pressed();
	void _on_rules_pressed();
	void _on_track_pressed();
	void _on_pull_pressed();
	void _on_push_pressed();
	void _on_quota_pressed();

protected:
	static void _bind_methods();

public:
	explicit GoLfsDialog(go_suite::GoGitController *p_controller = nullptr);
};
