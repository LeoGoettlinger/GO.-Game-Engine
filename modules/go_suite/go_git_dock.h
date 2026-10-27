// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "go_git_controller.h"

#include "scene/gui/box_container.h"

class Button;
class ItemList;
class Label;
class LineEdit;
class TextEdit;
class GoLfsDialog;
class GoRemoteAssistant;

class GoGitDock : public VBoxContainer {
	GDCLASS(GoGitDock, VBoxContainer);

	go_suite::GoGitController controller;
	GoRemoteAssistant *remote_dialog = nullptr;
	GoLfsDialog *lfs_dialog = nullptr;

	Label *host_label = nullptr;
	Label *state_label = nullptr;
	ItemList *changes_list = nullptr;
	TextEdit *output = nullptr;
	LineEdit *commit_input = nullptr;
	Button *refresh_button = nullptr;
	Button *commit_button = nullptr;
	Button *pull_button = nullptr;
	Button *push_button = nullptr;
	Button *auto_commit_button = nullptr;
	Button *remote_button = nullptr;
	Button *lfs_button = nullptr;
	Button *log_button = nullptr;
	Button *plugins_button = nullptr;

	void _append_output(const String &p_text);
	void _on_refresh_pressed();
	void _on_commit_pressed();
	void _on_pull_pressed();
	void _on_push_pressed();
	void _on_auto_commit_pressed();
	void _on_remote_pressed();
	void _on_lfs_pressed();
	void _on_log_pressed();
	void _on_plugins_pressed();

protected:
	static void _bind_methods();

public:
	void set_host_text(const String &p_text);
	void refresh_status();
	GoGitDock();
};
