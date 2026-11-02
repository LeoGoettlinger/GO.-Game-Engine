/**************************************************************************/
/*  test_go_git_close.h                                                   */
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

#include "../go_git_lifecycle.h"
#include "test_go_auto_flow.h"

#include "core/object/callable_mp.h"
#include "core/object/message_queue.h"

// Runs the actual offline commit controller, but substitutes only the final
// EditorNode quit continuation so this test never exits its own test process.
struct GoGitCloseTest : public Node {
	int resumed = 0;
	void resume() { ++resumed; }
	void check(bool p_change, bool p_close_anyway) {
		GoAutoCommitFlowTest fixture;
		fixture.setup();
		if (p_change) {
			fixture.write("close.gd", "extends Node\n");
		}
		GoGitLifecycle *lifecycle = memnew(GoGitLifecycle(Callable()));
		lifecycle->startup_done = true;
		lifecycle->project = go_suite::to_std_string(fixture.directory);
		fixture.root->add_child(lifecycle);
		lifecycle->set_process(false);
		lifecycle->closing = true;
		lifecycle->resume_quit = callable_mp(this, &GoGitCloseTest::resume);
		CHECK_FALSE(GoSuiteRegistry::is_auto_commit_available());
		GoSuiteRegistry::set_auto_commit_callback(&GoAutoCommitController::request);
		GoSuiteRegistry::set_quit_pending(true);
		CHECK_FALSE(fixture.controller->start()); // Ordinary requests cannot bypass a pending quit.
		lifecycle->_custom_action("auto_commit");
		CHECK(lifecycle->automatic_running);
		CHECK(GoSuiteRegistry::is_quit_pending());
		fixture.run(p_change, true);
		CHECK(resumed == 0); // Completion is deliberately deferred beyond the controller stack.
		MessageQueue::get_singleton()->flush();
		if (p_change) {
			CHECK(resumed == 1);
			CHECK_FALSE(lifecycle->closing);
			CHECK_FALSE(GoSuiteRegistry::is_quit_pending());
		} else {
			CHECK(resumed == 0);
			CHECK(lifecycle->closing);
			CHECK(GoSuiteRegistry::is_quit_pending());
			if (p_close_anyway) {
				lifecycle->_confirmed();
				CHECK(resumed == 1);
			} else {
				lifecycle->_cancelled();
				CHECK(resumed == 0);
			}
			CHECK_FALSE(GoSuiteRegistry::is_quit_pending());
		}
		lifecycle->_automatic_finished(true, "stale duplicate completion");
		CHECK(resumed == (p_change || p_close_anyway ? 1 : 0));
		GoSuiteRegistry::set_auto_commit_callback(nullptr);
		memdelete(lifecycle);
	}
};

TEST_CASE("[GO.][GitClose][Editor] Successful commit resumes closing only after unlocking") {
	GoGitCloseTest *test = memnew(GoGitCloseTest);
	test->check(true, false);
	memdelete(test);
}

TEST_CASE("[GO.][GitClose][Editor] Failed commit keeps editor open until cancel or explicit close") {
	GoGitCloseTest *cancel = memnew(GoGitCloseTest);
	cancel->check(false, false);
	memdelete(cancel);
	GoGitCloseTest *close = memnew(GoGitCloseTest);
	close->check(false, true);
	memdelete(close);
}
