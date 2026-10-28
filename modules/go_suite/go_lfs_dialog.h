/**************************************************************************/
/*  go_lfs_dialog.h                                                       */
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
class TextEdit;
class OptionButton;
class SpinBox;
class ItemList;
class FileDialog;

class GoLfsDialog : public AcceptDialog {
	GDCLASS(GoLfsDialog, AcceptDialog);

	go_suite::GoGitController *controller = nullptr;
	Label *state_label = nullptr;
	TextEdit *output_view = nullptr;
	Button *check_button = nullptr;
	Button *install_button = nullptr;
	Button *init_button = nullptr;
	Button *rules_button = nullptr;
	Button *track_button = nullptr;
	Button *pull_button = nullptr;
	Button *push_button = nullptr;
	Button *quota_button = nullptr;

	OptionButton *mode_picker = nullptr;
	SpinBox *threshold = nullptr;
	ItemList *folder_list = nullptr;
	FileDialog *folder_picker = nullptr;
	void _on_add_folder();
	void _on_folder_selected(const String &p_path);
	void _on_remove_folder();
	void _on_save_policy();
	void _on_mode_selected(int p_index);
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
