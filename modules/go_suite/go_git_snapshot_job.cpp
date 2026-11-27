/**************************************************************************/
/*  go_git_snapshot_job.cpp                                               */
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

#include "go_git_snapshot_job.h"

#include "go_suite_core.h"

#include "core/os/os.h"

void GoGitSnapshotJob::fail(const std::string &p_error) {
	process.cancel();
	active = false;
	result.is_repo = false;
	result.error = p_error;
}

bool GoGitSnapshotJob::start(const std::string &p_project) {
	if (active) {
		return false;
	}
	result = go_suite::GoGitSnapshot();
	if (p_project.empty()) {
		fail("No project path");
		return false;
	}
	commands = {
		go::rev_parse_verify_repo(p_project),
		go::git(p_project, { "status", "--porcelain=v1", "-z", "--branch", "--untracked-files=all" }),
		go::branch_show_current(p_project),
		go::remote_get_url(p_project),
		go::config_get(p_project, "user.name"),
		go::config_get(p_project, "user.email"),
		go::lfs_version(),
		go::lfs_env(p_project),
		go::lfs_ls_files_size(p_project),
		go::lfs_status(p_project),
		go::log(p_project, 10),
	};
	phase = 0;
	started = OS::get_singleton()->get_ticks_msec();
	active = process.start(commands[phase], 30000);
	if (!active) {
		fail(go_suite::to_std_string(process.get_output()));
	}
	return active;
}

bool GoGitSnapshotJob::poll() {
	if (!active) {
		return true;
	}
	if (OS::get_singleton()->get_ticks_msec() - started > 60000) {
		fail("Git status refresh timed out");
		return true;
	}
	if (!process.poll()) {
		return false;
	}
	if (process.has_process_error() || process.is_output_truncated()) {
		fail("Git status process failed, timed out or exceeded its output limit");
		return true;
	}
	const bool ok = process.succeeded();
	const std::string output = process.get_stdout_bytes();
	if (phase == 0) {
		if (!ok || go::trim(output) != "true") {
			fail("The project is not inside a Git working tree");
			return true;
		}
		result.is_repo = true;
	} else if (phase == 1) {
		if (!ok) {
			fail("Cannot read Git working-tree status");
			return true;
		}
		std::string error;
		result.status = go::parse_status_porcelain_z(output, error);
		if (!error.empty()) {
			fail(error);
			return true;
		}
		result.branch = result.status.branch;
	} else if (phase == 2 && ok && result.branch.empty()) {
		result.branch = go::parse_branch_name(output);
	} else if (phase == 3 && ok) {
		result.remote = go::parse_remote(go::trim(output));
	} else if (phase == 4 && ok) {
		go::parse_config_value(output, result.user_name);
	} else if (phase == 5 && ok) {
		go::parse_config_value(output, result.user_email);
	} else if (phase == 6) {
		result.lfs_installed = ok;
		if (!ok) {
			phase = 9;
		} // LFS is optional; skip its dependent queries.
	} else if (phase == 7) {
		result.lfs_ready = ok;
	} else if (phase == 8 && ok) {
		result.lfs = go::parse_lfs_ls_files(output);
	} else if (phase == 9 && ok) {
		go::parse_lfs_status(output, result.lfs_staged, result.lfs_pending);
	} else if (phase == 10 && ok) {
		result.recent_log = go::parse_log(output);
	}
	if (++phase == commands.size()) {
		active = false;
		return true;
	}
	if (!process.start(commands[phase], 30000)) {
		fail(go_suite::to_std_string(process.get_output()));
		return true;
	}
	return false;
}

void GoGitSnapshotJob::cancel() {
	if (active) {
		fail("Git status refresh cancelled");
	}
}
