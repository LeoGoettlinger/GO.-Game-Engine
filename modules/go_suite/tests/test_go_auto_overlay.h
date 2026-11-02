/**************************************************************************/
/*  test_go_auto_overlay.h                                                */
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
#include "../go_suite_registry.h"

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

struct GoAutoCommitControllerTest {
	static void check_windows() {
		Window *root = SceneTree::get_singleton()->get_root();
		const bool original_input = root->is_input_disabled();
		root->set_disable_input(false);
		Window *existing = memnew(Window);
		existing->hide();
		root->add_child(existing);
		SubViewport *disabled = memnew(SubViewport);
		disabled->set_disable_input(true);
		existing->add_child(disabled);
		GoAutoCommitController *controller = memnew(GoAutoCommitController);
		root->add_child(controller);
		controller->running = true;
		REQUIRE(controller->_show_overlay());
		CHECK(existing->is_input_disabled());
		CHECK(disabled->is_input_disabled());
		Window *added = memnew(Window);
		added->hide();
		root->add_child(added);
		CHECK(added->is_input_disabled());
		SubViewport *deleted = memnew(SubViewport);
		root->add_child(deleted);
		CHECK(deleted->is_input_disabled());
		memdelete(deleted); // Unlock must not dereference deleted viewports.
		controller->_finish(false, "simulated failure across windows");
		CHECK_FALSE(existing->is_input_disabled());
		CHECK_FALSE(added->is_input_disabled());
		CHECK(disabled->is_input_disabled());
		CHECK_FALSE(root->is_input_disabled());
		// The subscription is gone after unlock; future windows stay interactive.
		SubViewport *later = memnew(SubViewport);
		root->add_child(later);
		CHECK_FALSE(later->is_input_disabled());
		memdelete(later);
		memdelete(added);
		memdelete(existing);
		memdelete(controller);
		root->set_disable_input(original_input);
	}

	static void check_lifetime() {
		Window *root = SceneTree::get_singleton()->get_root();
		const bool original_input = root->is_input_disabled();
		root->set_disable_input(false);
		GoAutoCommitController *controller = memnew(GoAutoCommitController);
		root->add_child(controller);
		Node *original_parent = controller->get_parent();
		controller->running = true;
		CHECK(controller->_show_overlay());
		CHECK(controller->running);
		CHECK(controller->get_parent() == original_parent);
		CHECK(GoAutoCommitController::singleton == controller);
		CHECK(GoSuiteRegistry::is_editor_input_locked());
		CHECK(GoSuiteRegistry::is_git_job_busy());
		CHECK(root->is_input_disabled());
		controller->_finish(false, "simulated Git failure");
		CHECK_FALSE(controller->running);
		CHECK_FALSE(GoSuiteRegistry::is_editor_input_locked());
		CHECK_FALSE(GoSuiteRegistry::is_git_job_busy());
		CHECK_FALSE(root->is_input_disabled());
		// Preserve a pre-existing disabled-input state, rather than unconditionally enabling it.
		root->set_disable_input(true);
		controller->running = true;
		CHECK(controller->_show_overlay());
		controller->_finish(true, "simulated success");
		CHECK(root->is_input_disabled());
		root->set_disable_input(false);
		controller->running = true;
		CHECK(controller->_show_overlay());
		memdelete(controller);
		CHECK_FALSE(GoSuiteRegistry::is_editor_input_locked());
		CHECK_FALSE(GoSuiteRegistry::is_git_job_busy());
		CHECK_FALSE(root->is_input_disabled());
		root->set_disable_input(original_input);
	}
};

TEST_CASE("[GO.][AutoOverlay][Editor] Overlay relocation, failure and teardown restore input") {
	GoAutoCommitControllerTest::check_lifetime();
}

TEST_CASE("[GO.][AutoOverlay][Editor] Existing, new and deleted window viewports preserve input state") {
	GoAutoCommitControllerTest::check_windows();
}
