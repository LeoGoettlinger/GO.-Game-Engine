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
		arguments.push_back(to_godot_string(argument));
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
