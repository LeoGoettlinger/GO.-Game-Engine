/**************************************************************************/
/*  go_system_git_runner.cpp                                              */
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

#include "go_system_git_runner.h"

#include "core/error/error_list.h"
#include "core/os/os.h"
#include "core/templates/list.h"

namespace go_suite {

std::string to_std_string(const String &p_string) {
	const CharString utf8 = p_string.utf8();
	return std::string(utf8.get_data());
}

String to_godot_string(const std::string &p_string) {
	return String::utf8(p_string.c_str());
}

String to_git_process_argument(const std::string &p_argument) {
#ifdef WINDOWS_ENABLED
	// This Godot base concatenates CreateProcess arguments and does not quote
	// empty strings. Preserve an empty argv entry rather than silently turning
	// e.g. `git config key ""` into a config query. No shell is involved.
	if (p_argument.empty()) {
		return "\"\"";
	}
#endif
	return to_godot_string(p_argument);
}

GoSystemGitRunner::GoSystemGitRunner(const String &p_project_path) {
	set_project_path(p_project_path);
}

void GoSystemGitRunner::set_project_path(const String &p_project_path) {
	project_path = to_std_string(p_project_path);
}

bool GoSystemGitRunner::run(const go::GitCommand &p_command, std::string &r_output, int &r_exit_code) const {
	r_output.clear();
	r_exit_code = -1;
	if (!p_command.valid) {
		r_output = p_command.error;
		return false;
	}
	if (OS::get_singleton() == nullptr) {
		r_output = "OS singleton unavailable";
		return false;
	}

	List<String> arguments;
	for (const std::string &argument : p_command.arguments) {
		arguments.push_back(to_git_process_argument(argument));
	}

	String output;
	const Error error = OS::get_singleton()->execute(
			to_godot_string(p_command.executable), arguments, &output, &r_exit_code, true);
	r_output = to_std_string(output);
	return error == OK;
}

bool GoSystemGitRunner::run_git(const std::vector<std::string> &p_arguments, std::string &r_output, int &r_exit_code) const {
	return run(go::git(project_path, p_arguments), r_output, r_exit_code);
}

} // namespace go_suite
