/**************************************************************************/
/*  test_go_auto_flow.h                                                   */
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

#include "../go_auto_commit_controller.h"
#include "../go_suite_core.h"
#include "../go_suite_registry.h"
#include "../go_system_git_runner.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

// Uses actual native pipes and local repositories. Never calls the AI gateway.
struct GoAutoCommitFlowTest {
	String directory = TestUtils::get_temp_path("go-auto-flow-" + itos(OS::get_singleton()->get_ticks_usec()));
	Window *root = SceneTree::get_singleton()->get_root();
	bool original_input = root->is_input_disabled();
	Variant original_lfs = ProjectSettings::get_singleton()->get_setting("go/git_suite/lfs/enabled", false);
	bool had_lfs = ProjectSettings::get_singleton()->has_setting("go/git_suite/lfs/enabled");
	go_suite::GoGitController git_controller;
	GoAutoCommitController *controller = nullptr;
	std::string output;

	GoAutoCommitFlowTest() {
		DirAccess::make_dir_recursive_absolute(directory);
		ProjectSettings::get_singleton()->set_setting("go/git_suite/lfs/enabled", false);
		root->set_disable_input(false);
		git_controller.set_project(directory);
		controller = memnew(GoAutoCommitController);
		controller->set_git_controller(&git_controller);
		controller->test_message_fallback = true;
		root->add_child(controller);
	}
	~GoAutoCommitFlowTest() {
		if (controller != nullptr) {
			memdelete(controller);
		}
		root->set_disable_input(original_input);
		ProjectSettings::get_singleton()->set_setting("go/git_suite/lfs/enabled", had_lfs ? original_lfs : Variant());
		Ref<DirAccess> access = DirAccess::open(directory);
		if (access.is_valid()) {
			access->erase_contents_recursive();
			DirAccess::remove_absolute(directory);
		}
	}
	bool git(const std::vector<std::string> &p_args) {
		go_suite::GoSystemGitRunner runner;
		int exit = -1;
		output.clear();
		return runner.run(go::git(go_suite::to_std_string(directory), p_args), output, exit) && exit == 0;
	}
	void write(const String &p_file, const String &p_text) {
		Ref<FileAccess> file = FileAccess::open(directory.path_join(p_file), FileAccess::WRITE);
		REQUIRE(file.is_valid());
		file->store_string(p_text);
	}
	void run(bool p_success, bool p_already_started = false) {
		if (!p_already_started) {
			REQUIRE(controller->start());
		}
		CHECK(GoSuiteRegistry::is_git_job_busy());
		const uint64_t start = OS::get_singleton()->get_ticks_msec();
		while (controller->running && OS::get_singleton()->get_ticks_msec() - start < 30000) {
			controller->_notification(Node::NOTIFICATION_PROCESS);
			OS::get_singleton()->delay_usec(1000);
		}
		INFO(go_suite::to_std_string(controller->last_result));
		REQUIRE_FALSE(controller->running);
		CHECK(controller->last_success == p_success);
		CHECK_FALSE(GoSuiteRegistry::is_git_job_busy());
		CHECK_FALSE(GoSuiteRegistry::is_editor_input_locked());
		CHECK_FALSE(root->is_input_disabled());
	}
	void setup() {
		REQUIRE(git({ "init", "--initial-branch=main" }));
		REQUIRE(git({ "config", "user.name", "GO Native Test" }));
		REQUIRE(git({ "config", "user.email", "go-test@example.invalid" }));
		REQUIRE(git({ "config", "commit.gpgsign", "false" }));
	}
};

TEST_CASE("[GO.][AutoFlow][Editor] No changes, local commit, successful and rejected push release locks") {
	GoAutoCommitFlowTest fixture;
	fixture.setup();
	fixture.run(false);
	fixture.write("scene file.gd", "extends Node\n");
	fixture.run(true);
	REQUIRE(fixture.git({ "log", "--format=%s", "-1" }));
	CHECK_FALSE(fixture.output.empty());
	REQUIRE(fixture.git({ "status", "--porcelain" }));
	CHECK(fixture.output.empty());
	// Repositories inside .git cannot accidentally become staged project files.
	const std::string bare = go_suite::to_std_string(fixture.directory.path_join(".git/remote.git"));
	REQUIRE(fixture.git({ "init", "--bare", bare }));
	REQUIRE(fixture.git({ "remote", "add", "origin", bare }));
	fixture.write("scene file.gd", "extends Node\n# second change\n");
	fixture.run(true);
	REQUIRE(fixture.git({ "--git-dir=" + bare, "rev-parse", "refs/heads/main" }));
	const std::string remote_head = fixture.output;
	REQUIRE(fixture.git({ "rev-parse", "HEAD" }));
	CHECK(fixture.output == remote_head);
	const std::string checked_out = go_suite::to_std_string(fixture.directory.path_join(".git/checked-out"));
	REQUIRE(fixture.git({ "init", "--initial-branch=main", checked_out }));
	REQUIRE(fixture.git({ "remote", "set-url", "origin", checked_out }));
	fixture.write("scene file.gd", "extends Node\n# rejected remote, retain local commit\n");
	fixture.run(false);
	REQUIRE(fixture.git({ "rev-list", "--count", "HEAD" }));
	CHECK(go::trim(fixture.output) == "3");
	REQUIRE(fixture.git({ "status", "--porcelain" }));
	CHECK(fixture.output.empty());
}

TEST_CASE("[GO.][AutoFlow][Editor] Invalid staged LFS content prevents a commit") {
	GoAutoCommitFlowTest fixture;
	fixture.setup();
	// Explicitly disable inherited filters to simulate a missing/misconfigured LFS installation.
	REQUIRE(fixture.git({ "config", "filter.lfs.process", "" }));
	REQUIRE(fixture.git({ "config", "filter.lfs.clean", "" }));
	REQUIRE(fixture.git({ "config", "filter.lfs.required", "false" }));
	fixture.write(".gitattributes", "*.png filter=lfs diff=lfs merge=lfs -text\n");
	fixture.write("asset.png", "not a pointer");
	fixture.run(false);
	CHECK_FALSE(fixture.git({ "rev-parse", "--verify", "HEAD" }));
}

TEST_CASE("[GO.][AutoFlow][Editor] Real LFS clean filter and staged pointer verification") {
	GoAutoCommitFlowTest fixture;
	fixture.setup();
	REQUIRE(fixture.git({ "lfs", "version" }));
	ProjectSettings::get_singleton()->set_setting("go/git_suite/lfs/enabled", true);
	fixture.write("asset.png", "LFS payload for the native transaction fixture");
	fixture.run(true);
	REQUIRE(fixture.git({ "show", "HEAD:asset.png" }));
	CHECK(go::is_lfs_pointer_content(fixture.output));
}

TEST_CASE("[GO.][AutoFlow][Editor] Preflight cancellation releases only its own job reservation") {
	GoAutoCommitFlowTest fixture;
	fixture.setup();
	fixture.write("new.gd", "extends Node\n");
	REQUIRE(fixture.controller->start());
	CHECK(GoSuiteRegistry::is_git_job_busy());
	CHECK_FALSE(GoSuiteRegistry::is_editor_input_locked());
	memdelete(fixture.controller);
	fixture.controller = nullptr;
	CHECK_FALSE(GoSuiteRegistry::is_git_job_busy());
	CHECK_FALSE(GoSuiteRegistry::is_editor_input_locked());
	CHECK_FALSE(fixture.root->is_input_disabled());
}
