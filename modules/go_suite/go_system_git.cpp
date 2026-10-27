/**************************************************************************/
/*  go_system_git.cpp                                                     */
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

#include "go_system_git.h"

#include "go_suite_core.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <set>
#include <sstream>

namespace go {

namespace {

bool has_nul(const std::string &p_text) {
	return p_text.find('\0') != std::string::npos;
}

bool has_line_break(const std::string &p_text) {
	return p_text.find('\n') != std::string::npos || p_text.find('\r') != std::string::npos;
}

bool is_conflict_code(char p_index, char p_worktree) {
	if (p_index == 'U' || p_worktree == 'U') {
		return true;
	}
	if (p_index == 'A' && p_worktree == 'A') {
		return true;
	}
	if (p_index == 'D' && p_worktree == 'D') {
		return true;
	}
	return false;
}

int parse_int_inside(const std::string &p_text, const std::string &p_key) {
	const size_t pos = p_text.find(p_key);
	if (pos == std::string::npos) {
		return 0;
	}
	const size_t start = pos + p_key.size();
	size_t end = start;
	while (end < p_text.size() && std::isdigit(static_cast<unsigned char>(p_text[end]))) {
		++end;
	}
	if (end == start) {
		return 0;
	}
	return std::atoi(p_text.substr(start, end - start).c_str());
}

} // namespace

GitCommand command(const std::string &p_executable, const std::vector<std::string> &p_arguments) {
	GitCommand result;
	result.executable = trim(p_executable);
	if (result.executable.empty()) {
		result.error = "empty executable";
		return result;
	}
	for (const std::string &argument : p_arguments) {
		if (!is_safe_argument(argument)) {
			result.error = "unsafe argument";
			return result;
		}
		result.arguments.push_back(argument);
	}
	result.valid = true;
	result.display = command_display(result);
	return result;
}

GitCommand git(const std::string &p_project, const std::vector<std::string> &p_arguments) {
	if (!is_safe_project_path(p_project)) {
		GitCommand result;
		result.error = "unsafe project path";
		return result;
	}
	std::vector<std::string> arguments = { "-C", p_project };
	arguments.insert(arguments.end(), p_arguments.begin(), p_arguments.end());
	return command("git", arguments);
}

GitCommand git_lfs(const std::string &p_project, const std::vector<std::string> &p_arguments) {
	std::vector<std::string> arguments = { "lfs" };
	arguments.insert(arguments.end(), p_arguments.begin(), p_arguments.end());
	return git(p_project, arguments);
}

bool is_safe_project_path(const std::string &p_project) {
	const std::string project = trim(p_project);
	if (project.empty() || has_nul(project) || has_line_break(project)) {
		return false;
	}
	return project[0] != '-';
}

bool is_safe_argument(const std::string &p_argument) {
	return !has_nul(p_argument);
}

bool is_safe_remote_argument(const std::string &p_url) {
	const std::string url = trim(p_url);
	if (url.empty() || url[0] == '-' || has_line_break(url) || has_nul(url)) {
		return false;
	}
	return is_safe_remote_url(url);
}

std::string quote_argument(const std::string &p_argument) {
	if (p_argument.empty()) {
		return "\"\"";
	}
	bool needs_quotes = false;
	for (const char c : p_argument) {
		if (std::isspace(static_cast<unsigned char>(c)) || c == '"' || c == '\\') {
			needs_quotes = true;
			break;
		}
	}
	if (!needs_quotes) {
		return p_argument;
	}
	std::string quoted = "\"";
	for (const char c : p_argument) {
		if (c == '"' || c == '\\') {
			quoted += '\\';
		}
		quoted += c;
	}
	quoted += '"';
	return quoted;
}

std::string command_display(const GitCommand &p_command) {
	if (!p_command.valid) {
		return p_command.error;
	}
	std::vector<std::string> parts;
	parts.push_back(quote_argument(p_command.executable));
	for (const std::string &argument : p_command.arguments) {
		parts.push_back(quote_argument(argument));
	}
	return join(parts, " ");
}

GitCommand status(const std::string &p_project) {
	return git(p_project, { "status", "--porcelain=v1", "--branch", "--untracked-files=all" });
}

GitCommand diff_stat(const std::string &p_project) {
	return git(p_project, { "diff", "--stat", "--no-ext-diff", "--no-color" });
}

GitCommand diff_text(const std::string &p_project, size_t p_max_chars) {
	(void)p_max_chars; // Truncation is applied to the captured output, not to the argv.
	return git(p_project, { "diff", "--no-ext-diff", "--no-color", "--no-renames", "--unified=3" });
}

GitCommand log(const std::string &p_project, int p_limit) {
	const int limit = std::clamp(p_limit, 1, 200);
	return git(p_project, { "log", "-n", std::to_string(limit), "--date=short", "--pretty=format:%H%x1f%an%x1f%ad%x1f%s%x1e" });
}

GitCommand branch_show_current(const std::string &p_project) {
	return git(p_project, { "branch", "--show-current" });
}

GitCommand rev_parse_verify_repo(const std::string &p_project) {
	return git(p_project, { "rev-parse", "--is-inside-work-tree" });
}

GitCommand remote_get_url(const std::string &p_project, const std::string &p_remote) {
	return git(p_project, { "remote", "get-url", p_remote });
}

GitCommand ahead_behind(const std::string &p_project, const std::string &p_upstream) {
	if (p_upstream.empty() || p_upstream[0] == '-' || has_nul(p_upstream)) {
		GitCommand result;
		result.error = "unsafe upstream";
		return result;
	}
	return git(p_project, { "rev-list", "--left-right", "--count", "HEAD..." + p_upstream });
}

GitCommand config_get(const std::string &p_project, const std::string &p_key) {
	return git(p_project, { "config", "--get", p_key });
}

GitCommand lfs_version() {
	return command("git", { "lfs", "version" });
}

GitCommand lfs_ls_files_size(const std::string &p_project) {
	return git_lfs(p_project, { "ls-files", "--size" });
}

GitCommand lfs_status(const std::string &p_project) {
	return git_lfs(p_project, { "status" });
}

GitCommand lfs_env(const std::string &p_project) {
	return git_lfs(p_project, { "env" });
}

GitCommand remote_add(const std::string &p_project, const std::string &p_remote, const std::string &p_url) {
	if (!is_safe_remote_argument(p_url)) {
		GitCommand result;
		result.error = "unsafe remote URL";
		return result;
	}
	return git(p_project, { "remote", "add", p_remote, p_url });
}

GitCommand remote_set_url(const std::string &p_project, const std::string &p_remote, const std::string &p_url) {
	if (!is_safe_remote_argument(p_url)) {
		GitCommand result;
		result.error = "unsafe remote URL";
		return result;
	}
	return git(p_project, { "remote", "set-url", p_remote, p_url });
}

GitCommand fetch(const std::string &p_project, const std::string &p_remote) {
	return git(p_project, { "fetch", p_remote });
}

GitCommand pull(const std::string &p_project, const std::string &p_remote, const std::string &p_branch) {
	std::vector<std::string> arguments = { "pull", "--ff-only", p_remote };
	if (!p_branch.empty()) {
		arguments.push_back(p_branch);
	}
	return git(p_project, arguments);
}

GitCommand push(const std::string &p_project, const std::string &p_remote, const std::string &p_branch, bool p_set_upstream) {
	std::vector<std::string> arguments = { "push" };
	if (p_set_upstream) {
		arguments.push_back("-u");
	}
	arguments.push_back(p_remote);
	if (!p_branch.empty()) {
		arguments.push_back(p_branch);
	}
	return git(p_project, arguments);
}

GitCommand add_all(const std::string &p_project) {
	return git(p_project, { "add", "-A" });
}

GitCommand add_paths(const std::string &p_project, const std::vector<std::string> &p_paths) {
	std::vector<std::string> arguments = { "add", "--" };
	arguments.insert(arguments.end(), p_paths.begin(), p_paths.end());
	return git(p_project, arguments);
}

GitCommand commit(const std::string &p_project, const std::string &p_message) {
	if (trim(p_message).empty()) {
		GitCommand result;
		result.error = "empty commit message";
		return result;
	}
	return git(p_project, { "commit", "-m", p_message });
}

GitCommand lfs_install_local(const std::string &p_project) {
	return git_lfs(p_project, { "install", "--local" });
}

GitCommand lfs_install_global() {
	return command("git", { "lfs", "install" });
}

GitCommand lfs_track(const std::string &p_project, const std::string &p_pattern) {
	return git_lfs(p_project, { "track", p_pattern });
}

GitCommand lfs_track_many(const std::string &p_project, const std::vector<std::string> &p_patterns) {
	std::vector<std::string> arguments = { "track" };
	arguments.insert(arguments.end(), p_patterns.begin(), p_patterns.end());
	return git_lfs(p_project, arguments);
}

GitCommand lfs_pull(const std::string &p_project, const std::string &p_remote) {
	return git_lfs(p_project, { "pull", p_remote });
}

GitCommand lfs_push(const std::string &p_project, const std::string &p_remote) {
	return git_lfs(p_project, { "push", "--all", p_remote });
}

GitCommand lfs_migrate_import(const std::string &p_project, const std::vector<std::string> &p_patterns) {
	if (p_patterns.empty()) {
		GitCommand result;
		result.error = "no migrate patterns";
		return result;
	}
	return git_lfs(p_project, { "migrate", "import", "--include=" + join(p_patterns, ","), "--everything" });
}

GitCommand lfs_install_linux() {
	return command("pkexec", { "apt-get", "install", "-y", "git-lfs" });
}

GitCommand lfs_install_windows() {
	return command("winget", { "install", "--id", "GitHub.GitLFS", "-e", "--accept-source-agreements", "--accept-package-agreements" });
}

GitStatus parse_status_porcelain(const std::string &p_output) {
	GitStatus result;
	const std::vector<std::string> lines = split(p_output, '\n');
	for (const std::string &raw_line : lines) {
		std::string line = raw_line;
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		if (line.empty()) {
			continue;
		}
		if (starts_with(line, "## ")) {
			const std::string branch_info = line.substr(3);
			const size_t dots = branch_info.find("...");
			result.branch = trim(dots == std::string::npos ? branch_info : branch_info.substr(0, dots));
			result.ahead = parse_int_inside(branch_info, "ahead ");
			result.behind = parse_int_inside(branch_info, "behind ");
			continue;
		}
		if (line.size() < 3) {
			continue;
		}
		GitStatusEntry entry;
		entry.index_status = line[0];
		entry.worktree_status = line[1];
		std::string path = line.substr(3);
		const size_t arrow = path.find(" -> ");
		if (arrow != std::string::npos) {
			path = path.substr(arrow + 4);
		}
		entry.path = trim(path);
		if (entry.path.empty()) {
			continue;
		}
		entry.staged = entry.index_status != ' ' && entry.index_status != '?';
		entry.unstaged = entry.worktree_status != ' ' && entry.worktree_status != '?';
		entry.untracked = entry.index_status == '?' && entry.worktree_status == '?';
		entry.conflict = is_conflict_code(entry.index_status, entry.worktree_status);
		result.entries.push_back(entry);
		result.merge_conflict = result.merge_conflict || entry.conflict;
	}
	result.has_changes = !result.entries.empty();
	return result;
}

std::string parse_branch_name(const std::string &p_output) {
	const std::vector<std::string> lines = split(p_output, '\n');
	if (lines.empty()) {
		return "";
	}
	std::string line = trim(lines[0]);
	if (starts_with(line, "## ")) {
		line = trim(line.substr(3));
	}
	const size_t dots = line.find("...");
	if (dots != std::string::npos) {
		line = line.substr(0, dots);
	}
	const size_t space = line.find(' ');
	if (space != std::string::npos) {
		line = line.substr(0, space);
	}
	return trim(line);
}

bool parse_ahead_behind(const std::string &p_output, int &r_ahead, int &r_behind) {
	r_ahead = 0;
	r_behind = 0;
	std::istringstream stream(trim(p_output));
	if (!(stream >> r_ahead)) {
		return false;
	}
	if (!(stream >> r_behind)) {
		r_behind = 0;
	}
	return true;
}

std::vector<GitLogEntry> parse_log(const std::string &p_output) {
	std::vector<GitLogEntry> entries;
	const std::vector<std::string> records = split(p_output, '\x1e');
	for (const std::string &record : records) {
		const std::string clean = trim(record);
		if (clean.empty()) {
			continue;
		}
		const std::vector<std::string> fields = split(clean, '\x1f');
		if (fields.size() < 4) {
			continue;
		}
		GitLogEntry entry;
		entry.hash = trim(fields[0]);
		entry.author = trim(fields[1]);
		entry.date = trim(fields[2]);
		entry.subject = trim(fields[3]);
		entries.push_back(entry);
	}
	return entries;
}

uint64_t parse_size_string(const std::string &p_text) {
	const std::string text = trim(p_text);
	if (text.empty()) {
		return 0;
	}
	size_t unit_start = 0;
	while (unit_start < text.size() && (std::isdigit(static_cast<unsigned char>(text[unit_start])) || text[unit_start] == '.')) {
		++unit_start;
	}
	double value = 0.0;
	try {
		value = std::stod(text.substr(0, unit_start));
	} catch (...) {
		return 0;
	}
	const std::string unit = lower(trim(text.substr(unit_start)));
	double multiplier = 1.0;
	if (unit == "kb" || unit == "kib") {
		multiplier = 1024.0;
	} else if (unit == "mb" || unit == "mib") {
		multiplier = 1024.0 * 1024.0;
	} else if (unit == "gb" || unit == "gib") {
		multiplier = 1024.0 * 1024.0 * 1024.0;
	} else if (unit == "tb" || unit == "tib") {
		multiplier = 1024.0 * 1024.0 * 1024.0 * 1024.0;
	} else if (!unit.empty() && unit != "b" && unit != "byte" && unit != "bytes") {
		return static_cast<uint64_t>(value);
	}
	return static_cast<uint64_t>(std::llround(value * multiplier));
}

LfsSnapshot parse_lfs_ls_files(const std::string &p_output) {
	LfsSnapshot snapshot;
	const std::vector<std::string> lines = split(p_output, '\n');
	for (const std::string &raw_line : lines) {
		std::string line = trim(raw_line);
		if (line.empty() || line.back() == '\r') {
			if (!line.empty()) {
				line.pop_back();
			}
		}
		if (line.empty()) {
			continue;
		}
		const size_t first_space = line.find(' ');
		if (first_space == std::string::npos || first_space + 2 >= line.size()) {
			continue;
		}
		LfsFile file;
		file.oid = line.substr(0, first_space);
		file.materialized = line[first_space + 1] == '*';
		const size_t path_start = first_space + 3;
		const size_t paren = line.rfind(" (");
		if (paren == std::string::npos || paren <= path_start) {
			continue;
		}
		file.path = trim(line.substr(path_start, paren - path_start));
		const std::string size_text = line.substr(paren + 2, line.size() - paren - 3);
		file.size_bytes = parse_size_string(size_text);
		snapshot.files.push_back(file);
		snapshot.total_bytes += file.size_bytes;
	}
	std::vector<LfsFile> sorted = snapshot.files;
	std::sort(sorted.begin(), sorted.end(), [](const LfsFile &a, const LfsFile &b) {
		return a.size_bytes > b.size_bytes;
	});
	const size_t count = std::min<size_t>(5, sorted.size());
	snapshot.largest.assign(sorted.begin(), sorted.begin() + count);
	return snapshot;
}

bool parse_lfs_status(const std::string &p_output, int &r_staged, int &r_pending) {
	r_staged = 0;
	r_pending = 0;
	bool push_section = false;
	const std::vector<std::string> lines = split(p_output, '\n');
	for (const std::string &raw_line : lines) {
		const std::string line = lower(trim(raw_line));
		if (contains(line, "objects to be pushed")) {
			push_section = true;
		} else if (contains(line, "objects to be committed")) {
			push_section = false;
		}
		if (contains(line, "oid:")) {
			if (push_section) {
				++r_pending;
			} else {
				++r_staged;
			}
		}
	}
	return true;
}

bool parse_config_value(const std::string &p_output, std::string &r_value) {
	r_value = trim(p_output);
	return !r_value.empty();
}

bool is_lfs_pointer_content(const std::string &p_content) {
	return is_lfs_pointer_prefix(p_content.substr(0, std::min<size_t>(p_content.size(), 128)));
}

std::vector<std::string> lfs_patterns_for_files(
		const std::vector<std::pair<std::string, uint64_t>> &p_files,
		uint64_t p_warning_mib,
		GitHost p_host,
		const std::vector<std::string> &p_patterns) {
	std::set<std::string> unique;
	std::vector<std::string> ordered;
	for (const auto &entry : p_files) {
		const LargeFileDecision decision = classify_large_file(entry.first, entry.second, p_warning_mib, p_host, p_patterns);
		if (decision.lfs_covered) {
			continue;
		}
		if (decision.action != LargeFileAction::TRACK_LFS && decision.action != LargeFileAction::BLOCK) {
			continue;
		}
		const std::string pattern = suggested_pattern(entry.first);
		if (pattern.empty() || unique.count(pattern) != 0) {
			continue;
		}
		unique.insert(pattern);
		ordered.push_back(pattern);
	}
	return ordered;
}

std::string summarize_large_files(
		const std::vector<std::pair<std::string, uint64_t>> &p_files,
		uint64_t p_warning_mib,
		GitHost p_host) {
	std::vector<std::string> lines;
	for (const auto &entry : p_files) {
		const LargeFileDecision decision = classify_large_file(entry.first, entry.second, p_warning_mib, p_host);
		if (decision.action == LargeFileAction::NONE) {
			continue;
		}
		lines.push_back(entry.first + ": " + human_size(entry.second) + " — " + large_file_action_name(decision.action) + ": " + decision.reason);
	}
	return join(lines, "\n");
}

} // namespace go
