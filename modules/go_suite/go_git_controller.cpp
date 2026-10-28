/**************************************************************************/
/*  go_git_controller.cpp                                                 */
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

#include "go_git_controller.h"

#include "go_commit_message.h"
#include "go_git_lfs.h"
#include "go_suite_core.h"

#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"

#include <algorithm>

namespace go_suite {

namespace {

std::string full_path(const std::string &p_project, const std::string &p_relative) {
	std::string base = go::trim(p_project);
	while (!base.empty() && (go::ends_with(base, "/") || go::ends_with(base, "\\"))) {
		base.pop_back();
	}
	return base + "/" + p_relative;
}

std::string read_text_file(const std::string &p_path) {
	const Ref<FileAccess> file = FileAccess::open(to_godot_string(p_path), FileAccess::READ);
	if (file.is_null()) {
		return "";
	}
	return to_std_string(file->get_as_utf8_string());
}

bool write_text_file(const std::string &p_path, const std::string &p_text) {
	const Ref<FileAccess> file = FileAccess::open(to_godot_string(p_path), FileAccess::WRITE);
	if (file.is_null()) {
		return false;
	}
	return file->store_string(to_godot_string(p_text));
}

} // namespace

void GoGitController::set_project(const String &p_project_path) {
	project_path = to_std_string(p_project_path);
	runner.set_project_path(p_project_path);
}

bool GoGitController::run_checked(const go::GitCommand &p_command, std::string &r_output, std::string &r_error) const {
	int exit_code = -1;
	if (!runner.run(p_command, r_output, exit_code)) {
		r_error = r_output.empty() ? p_command.error : r_output;
		return false;
	}
	if (exit_code != 0) {
		r_error = go::trim(r_output);
		if (r_error.empty()) {
			r_error = "Command failed: " + p_command.display;
		}
		return false;
	}
	return true;
}

bool GoGitController::has_upstream() const {
	std::string output;
	int exit_code = -1;
	const go::GitCommand command = go::git(project_path, { "rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{u}" });
	if (!runner.run(command, output, exit_code) || exit_code != 0) {
		return false;
	}
	return !go::trim(output).empty();
}

bool GoGitController::verify_staged_pointers(std::string &r_error) const {
	std::string status_output;
	int status_exit = -1;
	if (!runner.run(go::status(project_path), status_output, status_exit) || status_exit != 0) {
		r_error = "Could not read Git status for LFS pointer verification";
		return false;
	}
	const go::GitStatus status = go::parse_status_porcelain(status_output);
	for (const go::GitStatusEntry &entry : status.entries) {
		if (!entry.staged || entry.index_status == 'D') {
			continue;
		}
		std::string attribute;
		if (!run_checked(go::git(project_path, { "check-attr", "--cached", "filter", "--", entry.path }), attribute, r_error)) {
			return false;
		}
		if (!go::ends_with(go::trim(attribute), ": lfs")) {
			continue;
		}
		const std::string path = full_path(project_path, entry.path);
		if (!FileAccess::exists(to_godot_string(path))) {
			continue;
		}
		std::string staged;
		int show_exit = -1;
		const go::GitCommand show = go::git(project_path, { "show", ":" + entry.path });
		if (!runner.run(show, staged, show_exit) || show_exit != 0) {
			continue;
		}
		if (go::is_lfs_pointer_content(staged)) {
			continue;
		}
		std::string add_output;
		std::string add_error;
		if (!run_checked(go::add_paths(project_path, { entry.path }), add_output, add_error)) {
			r_error = add_error;
			return false;
		}
		if (!runner.run(show, staged, show_exit) || show_exit != 0 || !go::is_lfs_pointer_content(staged)) {
			r_error = "LFS pointer verification failed after re-staging: " + entry.path;
			return false;
		}
	}
	return true;
}

bool GoGitController::is_repo(std::string &r_error) const {
	if (project_path.empty()) {
		r_error = "No project path is set";
		return false;
	}
	std::string output;
	int exit_code = -1;
	const go::GitCommand command = go::rev_parse_verify_repo(project_path);
	if (!runner.run(command, output, exit_code) || exit_code != 0 || go::trim(output) != "true") {
		r_error = "The open project is not a Git repository";
		return false;
	}
	return true;
}

GoGitSnapshot GoGitController::snapshot(std::string &r_error) const {
	GoGitSnapshot snapshot;
	if (!is_repo(r_error)) {
		snapshot.error = r_error;
		return snapshot;
	}
	snapshot.is_repo = true;

	std::string output;
	int exit_code = -1;

	if (runner.run(go::status(project_path), output, exit_code) && exit_code == 0) {
		snapshot.status = go::parse_status_porcelain(output);
		snapshot.branch = snapshot.status.branch;
	}
	if (snapshot.branch.empty() && runner.run(go::branch_show_current(project_path), output, exit_code) && exit_code == 0) {
		snapshot.branch = go::parse_branch_name(output);
	}

	if (runner.run(go::remote_get_url(project_path), output, exit_code) && exit_code == 0) {
		snapshot.remote = go::parse_remote(go::trim(output));
	}

	if (runner.run(go::config_get(project_path, "user.name"), output, exit_code) && exit_code == 0) {
		go::parse_config_value(output, snapshot.user_name);
	}
	if (runner.run(go::config_get(project_path, "user.email"), output, exit_code) && exit_code == 0) {
		go::parse_config_value(output, snapshot.user_email);
	}

	if (runner.run(go::lfs_version(), output, exit_code) && exit_code == 0) {
		snapshot.lfs_installed = true;
		if (runner.run(go::lfs_env(project_path), output, exit_code) && exit_code == 0) {
			snapshot.lfs_ready = true;
		}
		if (runner.run(go::lfs_ls_files_size(project_path), output, exit_code) && exit_code == 0) {
			snapshot.lfs = go::parse_lfs_ls_files(output);
		}
		if (runner.run(go::lfs_status(project_path), output, exit_code) && exit_code == 0) {
			go::parse_lfs_status(output, snapshot.lfs_staged, snapshot.lfs_pending);
		}
	}

	if (runner.run(go::log(project_path, 10), output, exit_code) && exit_code == 0) {
		snapshot.recent_log = go::parse_log(output);
	}

	std::vector<std::pair<std::string, uint64_t>> sized_files;
	for (const go::GitStatusEntry &entry : snapshot.status.entries) {
		const int64_t size = FileAccess::get_size(to_godot_string(full_path(project_path, entry.path)));
		if (size > 0) {
			sized_files.push_back({ entry.path, static_cast<uint64_t>(size) });
		}
	}
	const go::LfsPolicy policy = lfs_policy();
	if (policy.mode == go::LfsMode::FILE_TYPES) {
		snapshot.suggested_lfs_patterns = go::lfs_patterns_for_files(sized_files, policy.threshold_mib, snapshot.remote.host_kind);
	} else if (policy.mode == go::LfsMode::FOLDERS) {
		snapshot.suggested_lfs_patterns = go::lfs_folder_patterns(policy.folders);
	} else {
		for (const auto &file : sized_files) {
			if (go::lfs_policy_selects(policy, file.first, file.second)) {
				snapshot.suggested_lfs_patterns.push_back(go::lfs_literal_pattern(file.first));
			}
		}
	}
	const std::string summary = go::summarize_large_files(sized_files, policy.threshold_mib, snapshot.remote.host_kind);
	if (!summary.empty()) {
		snapshot.large_file_warnings = go::split(summary, '\n');
	}

	snapshot.error.clear();
	r_error.clear();
	return snapshot;
}

bool GoGitController::set_remote(const std::string &p_url, std::string &r_error) const {
	if (!go::is_safe_remote_argument(p_url)) {
		r_error = "Remote URL is empty, unsafe or contains embedded credentials";
		return false;
	}
	std::string set_output;
	std::string set_error;
	if (run_checked(go::remote_set_url(project_path, "origin", p_url), set_output, set_error)) {
		return true;
	}
	std::string add_output;
	std::string add_error;
	if (run_checked(go::remote_add(project_path, "origin", p_url), add_output, add_error)) {
		return true;
	}
	r_error = set_error + " / " + add_error;
	return false;
}

bool GoGitController::fetch(std::string &r_output, std::string &r_error) const {
	return run_checked(go::fetch(project_path), r_output, r_error);
}

bool GoGitController::pull(std::string &r_output, std::string &r_error) const {
	std::string branch_output;
	int branch_exit = -1;
	if (!runner.run(go::branch_show_current(project_path), branch_output, branch_exit) || branch_exit != 0) {
		r_error = "Could not determine the current branch";
		return false;
	}
	const std::string branch = go::parse_branch_name(branch_output);
	return run_checked(go::pull(project_path, "origin", branch), r_output, r_error);
}

bool GoGitController::push(bool p_set_upstream, std::string &r_output, std::string &r_error) const {
	std::string remote_output;
	int remote_exit = -1;
	if (!runner.run(go::remote_get_url(project_path), remote_output, remote_exit) || remote_exit != 0) {
		r_error = "No remote configured — commit only, nothing was pushed";
		return false;
	}
	std::string branch_output;
	int branch_exit = -1;
	if (!runner.run(go::branch_show_current(project_path), branch_output, branch_exit) || branch_exit != 0) {
		r_error = "Could not determine the current branch";
		return false;
	}
	const std::string branch = go::parse_branch_name(branch_output);
	const bool set_upstream = p_set_upstream || !has_upstream();
	return run_checked(go::push(project_path, "origin", branch, set_upstream), r_output, r_error);
}

bool GoGitController::stage_all(std::string &r_error) const {
	if (!ensure_lfs_attributes(r_error)) {
		return false;
	}
	std::string output;
	if (!run_checked(go::add_all(project_path), output, r_error) ||
			!run_checked(go::git(project_path, { "add", "--renormalize", "--", "." }), output, r_error)) {
		return false;
	}
	return verify_staged_pointers(r_error);
}

bool GoGitController::stage_paths(const std::vector<std::string> &p_paths, std::string &r_error) const {
	if (!ensure_lfs_attributes(r_error)) {
		return false;
	}
	std::string output;
	return run_checked(go::add_paths(project_path, p_paths), output, r_error);
}

bool GoGitController::commit(const std::string &p_message, std::string &r_output, std::string &r_error) const {
	const std::string message = go::sanitize_commit_message(p_message);
	if (message.empty()) {
		r_error = "Commit message is empty";
		return false;
	}
	return run_checked(go::commit(project_path, message), r_output, r_error);
}

go::LfsPolicy GoGitController::lfs_policy() const {
	go::LfsPolicy policy;
	Ref<ConfigFile> config;
	config.instantiate();
	if (config->load(to_godot_string(full_path(project_path, ".godot/go_lfs.cfg"))) == OK) {
		int mode = config->get_value("lfs", "mode", 0);
		policy.mode = mode == 1 ? go::LfsMode::SIZE : (mode == 2 ? go::LfsMode::FOLDERS : go::LfsMode::FILE_TYPES);
		policy.threshold_mib = int64_t(config->get_value("lfs", "threshold_mib", 5));
		PackedStringArray folders = config->get_value("lfs", "folders", PackedStringArray());
		for (const String &folder : folders) {
			policy.folders.push_back(to_std_string(folder));
		}
	}
	return policy;
}

bool GoGitController::save_lfs_policy(const go::LfsPolicy &p_policy, std::string &r_error) const {
	if (p_policy.threshold_mib < 1 || p_policy.threshold_mib > 1048576) {
		r_error = "LFS threshold must be between 1 and 1048576 MiB";
		return false;
	}
	if (p_policy.mode == go::LfsMode::FOLDERS && p_policy.folders.empty()) {
		r_error = "Choose at least one project folder";
		return false;
	}
	Ref<DirAccess> directory = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	PackedStringArray folders;
	for (const std::string &folder : p_policy.folders) {
		if (!go::valid_lfs_folder(folder)) {
			r_error = "Invalid project folder: " + folder;
			return false;
		}
		std::string current = project_path;
		for (const std::string &part : go::split(folder, '/')) {
			current += "/" + part;
			if (directory->is_link(to_godot_string(current)) || !directory->dir_exists(to_godot_string(current))) {
				r_error = "Folder must exist inside the project and must not contain symlinks: " + folder;
				return false;
			}
		}
		folders.push_back(to_godot_string(folder));
	}
	Ref<ConfigFile> config;
	config.instantiate();
	config->set_value("lfs", "mode", int(p_policy.mode));
	config->set_value("lfs", "threshold_mib", int64_t(p_policy.threshold_mib));
	config->set_value("lfs", "folders", folders);
	const String cache = to_godot_string(full_path(project_path, ".godot"));
	if (directory->is_link(cache)) {
		r_error = "Refusing symlinked editor cache";
		return false;
	}
	directory->make_dir_recursive(cache);
	const String config_path = cache.path_join("go_lfs.cfg");
	if (directory->is_link(config_path) || config->save(config_path) != OK) {
		r_error = "Could not save project-local LFS settings";
		return false;
	}
	return true;
}

bool GoGitController::ensure_lfs_attributes(std::string &r_error) const {
	const std::string path = full_path(project_path, ".gitattributes");
	const std::string existing = read_text_file(path);
	const go::LfsPolicy policy = lfs_policy();
	std::vector<std::string> patterns;
	std::vector<std::string> exclusions;
	Ref<DirAccess> directory = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (directory->is_link(to_godot_string(path))) {
		r_error = "Refusing symlinked .gitattributes";
		return false;
	}
	if (policy.mode == go::LfsMode::FILE_TYPES) {
		patterns = go::default_lfs_patterns();
	} else if (policy.mode == go::LfsMode::FOLDERS) {
		for (const std::string &folder : policy.folders) {
			if (!go::valid_lfs_folder(folder)) {
				r_error = "Invalid saved LFS folder";
				return false;
			}
		}
		patterns = go::lfs_folder_patterns(policy.folders);
		exclusions = go::lfs_folder_patterns(policy.folders, true);
	} else {
		// Git supplies only tracked or unignored candidates; never recurse into .git.
		std::string files;
		if (!run_checked(go::git(project_path, { "-c", "core.quotePath=false", "ls-files", "--cached", "--others", "--exclude-standard" }), files, r_error)) {
			return false;
		}
		for (const std::string &file : go::split(files, '\n')) {
			if (file.empty()) {
				continue;
			}
			if (file.front() == '\"') {
				r_error = "Automatic LFS requires file names without control characters or quotes";
				return false;
			}
			std::string current = project_path;
			bool symlink = false;
			for (const std::string &part : go::split(file, '/')) {
				current += "/" + part;
				symlink = symlink || directory->is_link(to_godot_string(current));
			}
			if (symlink || !go::valid_lfs_folder(file)) {
				continue;
			}
			const int64_t size = FileAccess::get_size(to_godot_string(current));
			if (size >= 0 && go::lfs_policy_selects(policy, file, uint64_t(size))) {
				patterns.push_back(go::lfs_literal_pattern(file));
			}
		}
		std::sort(patterns.begin(), patterns.end());
		patterns.erase(std::unique(patterns.begin(), patterns.end()), patterns.end());
	}
	const std::string merged = go::merge_gitattributes(existing, patterns, exclusions);
	if (merged == existing) {
		return true;
	}
	if (!write_text_file(path, merged)) {
		r_error = "Could not write .gitattributes";
		return false;
	}
	return true;
}

bool GoGitController::lfs_install_local(std::string &r_output, std::string &r_error) const {
	return run_checked(go::lfs_install_local(project_path), r_output, r_error);
}

bool GoGitController::lfs_install_global(std::string &r_output, std::string &r_error) const {
	return run_checked(go::lfs_install_global(), r_output, r_error);
}

bool GoGitController::lfs_install_system(const std::string &p_platform, std::string &r_output, std::string &r_error) const {
	const go::GitCommand command = go::lower(p_platform) == "windows" ? go::lfs_install_windows() : go::lfs_install_linux();
	return run_checked(command, r_output, r_error);
}

bool GoGitController::lfs_track_patterns(const std::vector<std::string> &p_patterns, std::string &r_output, std::string &r_error) const {
	if (p_patterns.empty()) {
		r_error = "No LFS patterns to track";
		return false;
	}
	if (!ensure_lfs_attributes(r_error)) {
		return false;
	}
	return run_checked(go::lfs_track_many(project_path, p_patterns), r_output, r_error);
}

bool GoGitController::lfs_pull(std::string &r_output, std::string &r_error) const {
	return run_checked(go::lfs_pull(project_path), r_output, r_error);
}

bool GoGitController::lfs_push(std::string &r_output, std::string &r_error) const {
	return run_checked(go::lfs_push(project_path), r_output, r_error);
}

std::string GoGitController::diff_stat() const {
	std::string output;
	int exit_code = -1;
	if (runner.run(go::diff_stat(project_path), output, exit_code) && exit_code == 0) {
		return output;
	}
	return "";
}

std::string GoGitController::diff_text(size_t p_max_chars) const {
	std::string output;
	int exit_code = -1;
	if (!runner.run(go::diff_text(project_path, p_max_chars), output, exit_code) || exit_code != 0) {
		return "";
	}
	std::vector<std::string> changed_files;
	std::string status_output;
	int status_exit = -1;
	if (runner.run(go::status(project_path), status_output, status_exit) && status_exit == 0) {
		const go::GitStatus status = go::parse_status_porcelain(status_output);
		for (const go::GitStatusEntry &entry : status.entries) {
			changed_files.push_back(entry.path);
		}
	}
	return go::truncate_diff(output, p_max_chars, changed_files);
}

} // namespace go_suite
