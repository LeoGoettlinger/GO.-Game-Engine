/**************************************************************************/
/*  go_auto_commit_controller.h                                           */
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

#include "go_auto_commit.h"
#include "go_git_controller.h"

#include "scene/gui/control.h"

#include <string>

class ColorRect;
class GoKiloClient;
class Label;
class PanelContainer;
class ProgressBar;
class TextEdit;

class GoAutoCommitController : public Control {
	GDCLASS(GoAutoCommitController, Control);

	static GoAutoCommitController *singleton;

	go_suite::GoGitController *git_controller = nullptr;
	GoKiloClient *kilo_client = nullptr;
	go::AutoCommitStateMachine state;

	ColorRect *shade = nullptr;
	PanelContainer *panel = nullptr;
	Label *status_label = nullptr;
	ProgressBar *progress = nullptr;
	TextEdit *details = nullptr;

	std::string commit_message;
	std::string diff_stat;
	std::string diff_text;
	std::string status_text;
	std::string command_output;
	bool running = false;
	bool kilo_used = false;
	bool kilo_fallback = false;
	bool previous_input_disabled = false;

	void _build_ui();
	void _show_overlay();
	void _hide_overlay();
	void _update_overlay();
	void _append_details(const String &p_text);
	void _advance();
	void _finish(bool p_success, const String &p_message);
	void _on_chat_completed(const String &p_content, const String &p_tool_calls_json, bool p_used_json_fallback);
	void _on_kilo_failed(const String &p_message);
	void _on_request_cancelled();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	static bool request();

	void set_git_controller(go_suite::GoGitController *p_controller) { git_controller = p_controller; }
	bool start();
	bool is_running() const { return running; }

	GoAutoCommitController();
	~GoAutoCommitController() override;
};
