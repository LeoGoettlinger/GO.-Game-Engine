// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "go_git_controller.h"

#include "scene/gui/dialogs.h"

class Button;
class Label;
class LineEdit;
class OptionButton;

class GoRemoteAssistant : public ConfirmationDialog {
	GDCLASS(GoRemoteAssistant, ConfirmationDialog);

	go_suite::GoGitController *controller = nullptr;
	OptionButton *host_picker = nullptr;
	LineEdit *host_edit = nullptr;
	LineEdit *owner_edit = nullptr;
	LineEdit *repo_edit = nullptr;
	OptionButton *protocol_picker = nullptr;
	Label *url_preview = nullptr;
	Button *create_button = nullptr;

	void _on_host_selected(int p_index);
	void _on_protocol_selected(int p_index);
	void _on_fields_changed(const String &p_text);
	void _on_create_pressed();
	void _on_confirmed();
	void _update_preview();
	go::GitHost selected_host() const;
	std::string selected_protocol() const;
	std::string build_url() const;

protected:
	static void _bind_methods();

public:
	explicit GoRemoteAssistant(go_suite::GoGitController *p_controller = nullptr);
};
