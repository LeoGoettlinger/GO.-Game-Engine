/**************************************************************************/
/*  go_git_process.h                                                      */
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

#include "go_system_git.h"

#include "core/io/file_access.h"

// Main-thread-owned, nonblocking system Git process. No shell and no worker
// callbacks into deleted Controls. Poll once per frame while running.
class GoGitProcess {
	Ref<FileAccess> stdio_pipe;
	Ref<FileAccess> stderr_pipe;
	int64_t pid = -1;
	uint64_t started_msec = 0;
	uint64_t timeout_msec = 120000;
	std::string stdout_text;
	std::string stderr_text;
	std::string failure;
	int exit_code = -1;
	bool truncated = false;

	bool _drain(const Ref<FileAccess> &p_pipe, std::string &r_text);
	void _close();

public:
	GoGitProcess() = default;
	GoGitProcess(const GoGitProcess &) = delete;
	GoGitProcess &operator=(const GoGitProcess &) = delete;
	bool has_process_error() const { return !failure.empty(); }
	bool start(const go::GitCommand &p_command, uint64_t p_timeout_msec = 120000);
	bool poll(); // true once finished (also true when idle).
	void cancel();
	bool is_running() const { return pid >= 0; }
	bool succeeded() const { return !is_running() && failure.empty() && exit_code == 0; }
	String get_output() const;
	String get_stdout() const;
	~GoGitProcess();
};
