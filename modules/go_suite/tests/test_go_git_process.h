/**************************************************************************/
/*  test_go_git_process.h                                                 */
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

#include "../go_git_process.h"
#include "../go_system_git_runner.h"

#include "core/os/os.h"
#include "tests/test_macros.h"

namespace TestGoGitProcess {

inline bool wait_for_git(GoGitProcess &p_process) {
	const uint64_t start = OS::get_singleton()->get_ticks_msec();
	while (!p_process.poll()) {
		if (OS::get_singleton()->get_ticks_msec() - start > 10000) {
			p_process.cancel();
			return false;
		}
		OS::get_singleton()->delay_usec(1000);
	}
	return true;
}

TEST_CASE("[GO.][GitProcess] Native pipes return stdout and nonzero exit status") {
	const std::string project = go_suite::to_std_string(OS::get_singleton()->get_cwd());
	GoGitProcess process;
	REQUIRE(process.start(go::git(project, { "--version" })));
	CHECK_FALSE(process.start(go::git(project, { "--version" })));
	REQUIRE(wait_for_git(process));
	CHECK(process.succeeded());
	CHECK(process.get_stdout().contains("git version"));
	REQUIRE(process.start(go::git(project, { "go-invalid-subcommand-for-test" })));
	REQUIRE(wait_for_git(process));
	CHECK_FALSE(process.succeeded());
	CHECK_FALSE(process.get_output().is_empty());
	CHECK_FALSE(process.has_process_error());
}

TEST_CASE("[GO.][GitProcess] Waiting on stdin can be cancelled or timed out") {
	const std::string project = go_suite::to_std_string(OS::get_singleton()->get_cwd());
	GoGitProcess process;
	// No shell, hook or helper: git waits on its own open stdin pipe.
	REQUIRE(process.start(go::git(project, { "hash-object", "--stdin" })));
	process.cancel();
	CHECK_FALSE(process.is_running());
	CHECK_FALSE(process.succeeded());
	CHECK(process.has_process_error());
	REQUIRE(process.start(go::git(project, { "hash-object", "--stdin" }), 10));
	REQUIRE(wait_for_git(process));
	CHECK_FALSE(process.is_running());
	CHECK_FALSE(process.succeeded());
	CHECK(process.get_output().contains("timed out"));
	// Process reuse after both error paths must remain possible.
	REQUIRE(process.start(go::git(project, { "--version" })));
	REQUIRE(wait_for_git(process));
	CHECK(process.succeeded());
}

TEST_CASE("[GO.][GitProcess] Reject invalid commands without launching") {
	GoGitProcess process;
	CHECK_FALSE(process.start(go::GitCommand()));
	CHECK_FALSE(process.is_running());
	CHECK_FALSE(process.succeeded());
}

} // namespace TestGoGitProcess
