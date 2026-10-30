/**************************************************************************/
/*  go_git_process.cpp                                                    */
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

// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_git_process.h"

#include "go_system_git_runner.h"

#include "core/os/os.h"

bool GoGitProcess::start(const go::GitCommand &p_command, uint64_t p_timeout_msec) {
	if (is_running()) {
		return false;
	}
	stdout_text.clear();
	stderr_text.clear();
	failure.clear();
	exit_code = -1;
	truncated = false;
	if (!p_command.valid || p_command.executable != "git" || p_timeout_msec == 0) {
		failure = "Invalid system Git command";
		return false;
	}
	List<String> arguments;
	for (const std::string &argument : p_command.arguments) {
		arguments.push_back(go_suite::to_godot_string(argument));
	}
	const Dictionary process = OS::get_singleton()->execute_with_pipe("git", arguments, false);
	if (!process.has("pid") || !process.has("stdio") || !process.has("stderr")) {
		failure = "Could not start nonblocking system Git";
		return false;
	}
	pid = process["pid"];
	stdio_pipe = process["stdio"];
	stderr_pipe = process["stderr"];
	if (pid < 0 || stdio_pipe.is_null() || stderr_pipe.is_null()) {
		failure = "System Git returned invalid process handles";
		_close();
		return false;
	}
	started_msec = OS::get_singleton()->get_ticks_msec();
	timeout_msec = p_timeout_msec;
	return true;
}

bool GoGitProcess::_drain(const Ref<FileAccess> &p_pipe, std::string &r_text) {
	if (p_pipe.is_null()) {
		return false;
	}
	// Budget per frame and stream: drain even when the retained log is full,
	// otherwise a child writing to a full OS pipe could never finish.
	uint8_t buffer[4096];
	for (int i = 0; i < 16; ++i) {
		const uint64_t count = p_pipe->get_buffer(buffer, sizeof(buffer));
		if (count > sizeof(buffer)) {
			failure = "Could not read Git process output";
			return false;
		}
		if (count == 0) {
			return false;
		}
		constexpr size_t LIMIT = 1024 * 1024;
		const size_t keep = MIN(size_t(count), LIMIT - r_text.size());
		r_text.append(reinterpret_cast<const char *>(buffer), keep);
		truncated = truncated || keep < count;
	}
	return true;
}

bool GoGitProcess::poll() {
	if (!is_running()) {
		return true;
	}
	if (OS::get_singleton()->get_ticks_msec() - started_msec >= timeout_msec) {
		failure = "Git operation timed out; inspect repository status before retrying";
		_close();
		return true;
	}
	const bool more_stdout = _drain(stdio_pipe, stdout_text);
	const bool more_stderr = _drain(stderr_pipe, stderr_text);
	if (!failure.empty()) {
		_close();
		return true;
	}
	if (!OS::get_singleton()->is_process_running(pid)) {
		// Keep draining a completed process across frames if needed.
		if (more_stdout || more_stderr) {
			return false;
		}
		exit_code = OS::get_singleton()->get_process_exit_code(pid);
		pid = -1;
		stdio_pipe.unref();
		stderr_pipe.unref();
		return true;
	}
	return false;
}

void GoGitProcess::_close() {
	if (pid >= 0 && OS::get_singleton()->is_process_running(pid)) {
		OS::get_singleton()->kill(pid);
	}
	pid = -1;
	stdio_pipe.unref();
	stderr_pipe.unref();
}

void GoGitProcess::cancel() {
	if (is_running()) {
		failure = "Git operation cancelled; completed remote changes are not rolled back";
		_close();
	}
}

String GoGitProcess::get_stdout() const {
	return String::utf8(stdout_text.data(), stdout_text.size());
}

String GoGitProcess::get_output() const {
	std::string text = stdout_text + stderr_text;
	if (truncated) {
		text += "\n[Git output truncated]";
	}
	if (!failure.empty()) {
		text += "\n" + failure;
	}
	return String::utf8(text.data(), text.size());
}

GoGitProcess::~GoGitProcess() {
	_close();
}
