/**************************************************************************/
/*  go_git_controller.h                                                   */
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
#include "go_system_git_runner.h"

#include "core/string/ustring.h"

#include <string>
#include <vector>

namespace go_suite {

struct GoGitSnapshot {
	go::GitStatus status;
	go::GitRemoteInfo remote;
	go::LfsSnapshot lfs;
	std::vector<go::GitLogEntry> recent_log;
	std::string branch;
	std::string user_name;
	std::string user_email;
	std::string error;
	std::vector<std::string> large_file_warnings;
	std::vector<std::string> suggested_lfs_patterns;
	int lfs_staged = 0;
	int lfs_pending = 0;
	bool is_repo = false;
	bool lfs_installed = false;
	bool lfs_ready = false;
};

class GoGitController {
	std::string project_path;
	GoSystemGitRunner runner;

	bool run_checked(const go::GitCommand &p_command, std::string &r_output, std::string &r_error) const;
	bool has_upstream() const;
	bool verify_staged_pointers(std::string &r_error) const;

public:
	GoGitController() = default;

	void set_project(const String &p_project_path);
	const std::string &get_project_path() const { return project_path; }

	bool is_repo(std::string &r_error) const;
	GoGitSnapshot snapshot(std::string &r_error) const;

	bool set_remote(const std::string &p_url, std::string &r_error) const;
	bool fetch(std::string &r_output, std::string &r_error) const;
	bool pull(std::string &r_output, std::string &r_error) const;
	bool push(bool p_set_upstream, std::string &r_output, std::string &r_error) const;
	bool stage_all(std::string &r_error) const;
	bool stage_paths(const std::vector<std::string> &p_paths, std::string &r_error) const;
	bool commit(const std::string &p_message, std::string &r_output, std::string &r_error) const;

	bool ensure_lfs_attributes(std::string &r_error) const;
	bool lfs_install_local(std::string &r_output, std::string &r_error) const;
	bool lfs_install_global(std::string &r_output, std::string &r_error) const;
	bool lfs_install_system(const std::string &p_platform, std::string &r_output, std::string &r_error) const;
	bool lfs_track_patterns(const std::vector<std::string> &p_patterns, std::string &r_output, std::string &r_error) const;
	bool lfs_pull(std::string &r_output, std::string &r_error) const;
	bool lfs_push(std::string &r_output, std::string &r_error) const;

	std::string diff_stat() const;
	std::string diff_text(size_t p_max_chars = 12000) const;
};

} // namespace go_suite
