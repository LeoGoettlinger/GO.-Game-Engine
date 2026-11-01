/**************************************************************************/
/*  test_go_scene_undo.h                                                  */
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

#include "../go_agentics_tool_bridge.h"

#include "editor/editor_undo_redo_manager.h"
#include "scene/main/node.h"
#include "tests/test_macros.h"

namespace TestGoSceneUndo {

// Run this subset in its own process: no live EditorNode is needed. The test
// uses a fixed isolated history instead of looking up an open editor scene.
TEST_CASE("[GO.][SceneUndo] Add and rename restore scene ownership across undo and redo") {
	EditorUndoRedoManager *undo = memnew(EditorUndoRedoManager);
	Node *root = memnew(Node);
	Node *child = memnew(Node);
	root->set_name("Root");
	child->set_name("Child");
	constexpr int HISTORY = 7001;
	undo->create_action_for_history("Add", HISTORY);
	undo->force_fixed_history();
	GoAgenticsToolBridge::record_add_node(undo, root, child, root);
	undo->commit_action();
	CHECK(child->get_parent() == root);
	CHECK(child->get_owner() == root);
	CHECK(undo->undo_history(HISTORY));
	CHECK(child->get_parent() == nullptr);
	CHECK(undo->redo_history(HISTORY));
	CHECK(child->get_parent() == root);
	CHECK(child->get_owner() == root);
	undo->create_action_for_history("Rename", HISTORY);
	undo->force_fixed_history();
	GoAgenticsToolBridge::record_rename_node(undo, child, "Renamed");
	undo->commit_action();
	CHECK(child->get_name() == StringName("Renamed"));
	CHECK(undo->undo_history(HISTORY));
	CHECK(child->get_name() == StringName("Child"));
	CHECK(undo->redo_history(HISTORY));
	CHECK(child->get_name() == StringName("Renamed"));
	undo->clear_history(HISTORY);
	memdelete(root);
	memdelete(undo);
}

} // namespace TestGoSceneUndo
