/**************************************************************************/
/*  go_git_lifecycle.h                                                    */
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

#include "go_git_process.h"

#include "scene/main/node.h"

class ConfirmationDialog;
class CheckBox;
class Button;

class GoGitLifecycle : public Node {
	GDCLASS(GoGitLifecycle, Node);
	enum class Phase { IDLE,
		REMOTE,
		FETCH,
		STATUS,
		LFS };
	Phase phase = Phase::IDLE;
	GoGitProcess process;
	ConfirmationDialog *dialog = nullptr;
	CheckBox *dont_remind = nullptr;
	Callable resume_quit;
	Callable pull_action;
	Button *pull_button = nullptr;
	Button *git_suite_button = nullptr;
	go::GitStatus status;
	std::string project;
	bool startup_done = false;
	bool closing = false;
	bool allow_close = false;
	bool activation_offer = false;
	bool owns_job = false;
	bool status_failed = false;
	int pending_lfs = 0;
	bool _setting(const String &p_name, bool p_default) const;
	bool _save_setting(const String &p_name, bool p_value);
	void _release();
	void _launch(Phase p_phase, const go::GitCommand &p_command);
	void _poll();
	void _show_result();
	void _continue_quit();
	void _confirmed();
	void _cancelled();
	void _custom_action(const String &p_action);

protected:
	void _notification(int p_what);
	static void _bind_methods() {}

public:
	bool request_close(const Callable &p_resume);
	explicit GoGitLifecycle(const Callable &p_pull_action);
};
