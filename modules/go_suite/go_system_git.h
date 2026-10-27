/**************************************************************************/
/*  go_system_git.h                                                       */
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

#include "go_git_lfs.h"
#include "go_git_remote.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace go {

struct GitCommand {
	std::string executable;
	std::vector<std::string> arguments;
	std::string display;
	std::string error;
	bool valid = false;
};

struct GitStatusEntry {
	std::string path;
	char index_status = ' ';
	char worktree_status = ' ';
	bool staged = false;
	bool unstaged = false;
	bool untracked = false;
	bool conflict = false;
};

struct GitStatus {
	std::vector<GitStatusEntry> entries;
	std::string branch;
	int ahead = 0;
	int behind = 0;
	bool has_changes = false;
	bool merge_conflict = false;
};

struct GitLogEntry {
	std::string hash;
	std::string author;
	std::string date;
	std::string subject;
};

struct LfsFile {
	std::string oid;
	std::string path;
	uint64_t size_bytes = 0;
	bool materialized = true;
};

struct LfsSnapshot {
	std::vector<LfsFile> files;
	uint64_t total_bytes = 0;
	std::vector<LfsFile> largest;
};

// Low-level command construction. Every command is an argv vector; no shell is used.
GitCommand command(const std::string &p_executable, const std::vector<std::string> &p_arguments);
GitCommand git(const std::string &p_project, const std::vector<std::string> &p_arguments);
GitCommand git_lfs(const std::string &p_project, const std::vector<std::string> &p_arguments);
bool is_safe_project_path(const std::string &p_project);
bool is_safe_argument(const std::string &p_argument);
bool is_safe_remote_argument(const std::string &p_url);
std::string quote_argument(const std::string &p_argument);
std::string command_display(const GitCommand &p_command);

// Read-only commands.
GitCommand status(const std::string &p_project);
GitCommand diff_stat(const std::string &p_project);
GitCommand diff_text(const std::string &p_project, size_t p_max_chars = 12000);
GitCommand log(const std::string &p_project, int p_limit = 25);
GitCommand branch_show_current(const std::string &p_project);
GitCommand rev_parse_verify_repo(const std::string &p_project);
GitCommand remote_get_url(const std::string &p_project, const std::string &p_remote = "origin");
GitCommand ahead_behind(const std::string &p_project, const std::string &p_upstream);
GitCommand config_get(const std::string &p_project, const std::string &p_key);
GitCommand lfs_version();
GitCommand lfs_ls_files_size(const std::string &p_project);
GitCommand lfs_status(const std::string &p_project);
GitCommand lfs_env(const std::string &p_project);

// Write commands.
GitCommand remote_add(const std::string &p_project, const std::string &p_remote, const std::string &p_url);
GitCommand remote_set_url(const std::string &p_project, const std::string &p_remote, const std::string &p_url);
GitCommand fetch(const std::string &p_project, const std::string &p_remote = "origin");
GitCommand pull(const std::string &p_project, const std::string &p_remote = "origin", const std::string &p_branch = "");
GitCommand push(const std::string &p_project, const std::string &p_remote = "origin", const std::string &p_branch = "", bool p_set_upstream = false);
GitCommand add_all(const std::string &p_project);
GitCommand add_paths(const std::string &p_project, const std::vector<std::string> &p_paths);
GitCommand commit(const std::string &p_project, const std::string &p_message);
GitCommand lfs_install_local(const std::string &p_project);
GitCommand lfs_install_global();
GitCommand lfs_track(const std::string &p_project, const std::string &p_pattern);
GitCommand lfs_track_many(const std::string &p_project, const std::vector<std::string> &p_patterns);
GitCommand lfs_pull(const std::string &p_project, const std::string &p_remote = "origin");
GitCommand lfs_push(const std::string &p_project, const std::string &p_remote = "origin");
GitCommand lfs_migrate_import(const std::string &p_project, const std::vector<std::string> &p_patterns);

// Explicit, user-confirmed system installation helpers. Never called automatically.
GitCommand lfs_install_linux();
GitCommand lfs_install_windows();

// Output parsers.
GitStatus parse_status_porcelain(const std::string &p_output);
std::string parse_branch_name(const std::string &p_output);
bool parse_ahead_behind(const std::string &p_output, int &r_ahead, int &r_behind);
std::vector<GitLogEntry> parse_log(const std::string &p_output);
LfsSnapshot parse_lfs_ls_files(const std::string &p_output);
bool parse_lfs_status(const std::string &p_output, int &r_staged, int &r_pending);
bool parse_config_value(const std::string &p_output, std::string &r_value);
bool is_lfs_pointer_content(const std::string &p_content);
uint64_t parse_size_string(const std::string &p_text);

// Large-file remediation derived from the pure LFS rules.
std::vector<std::string> lfs_patterns_for_files(
		const std::vector<std::pair<std::string, uint64_t>> &p_files,
		uint64_t p_warning_mib,
		GitHost p_host,
		const std::vector<std::string> &p_patterns = default_lfs_patterns());
std::string summarize_large_files(
		const std::vector<std::pair<std::string, uint64_t>> &p_files,
		uint64_t p_warning_mib,
		GitHost p_host);

} // namespace go
