/**************************************************************************/
/*  go_remote_assistant.h                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

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
