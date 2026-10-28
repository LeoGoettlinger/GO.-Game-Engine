/**************************************************************************/
/*  go_git_dock.h                                                         */
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

#include "scene/gui/box_container.h"

class Button;
class ItemList;
class Label;
class LineEdit;
class TextEdit;
class GoAutoCommitController;
class GoLfsDialog;
class GoRemoteAssistant;

class GoGitDock : public VBoxContainer {
	GDCLASS(GoGitDock, VBoxContainer);

	go_suite::GoGitController controller;
	GoRemoteAssistant *remote_dialog = nullptr;
	GoLfsDialog *lfs_dialog = nullptr;
	GoAutoCommitController *auto_commit_controller = nullptr;

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
	void _on_auto_commit_finished(bool p_success, const String &p_message);
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
