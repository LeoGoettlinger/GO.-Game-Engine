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

#include "core/object/undo_redo.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/label.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#include <limits>

namespace TestGoSceneUndo {

// Test the shared operations on the real core history. EditorUndoRedoManager
// requires a live EditorNode/log/debugger even for a fixed history.
TEST_CASE("[GO.][SceneUndo] Add and rename restore scene ownership across undo and redo") {
	UndoRedo *undo = memnew(UndoRedo);
	Node *root = memnew(Node);
	Node *child = memnew(Node);
	root->set_name("Root");
	child->set_name("Child");
	undo->create_action("Add");
	GoAgenticsToolBridge::record_add_node(undo, root, child, root);
	undo->commit_action();
	CHECK(child->get_parent() == root);
	CHECK(child->get_owner() == root);
	CHECK(undo->undo());
	CHECK(child->get_parent() == nullptr);
	CHECK(undo->redo());
	CHECK(child->get_parent() == root);
	CHECK(child->get_owner() == root);
	undo->create_action("Rename");
	GoAgenticsToolBridge::record_rename_node(undo, child, "Renamed");
	undo->commit_action();
	CHECK(child->get_name() == StringName("Renamed"));
	CHECK(undo->undo());
	CHECK(child->get_name() == StringName("Child"));
	CHECK(undo->redo());
	CHECK(child->get_name() == StringName("Renamed"));
	undo->clear_history();
	memdelete(root);
	memdelete(undo);
}

TEST_CASE("[GO.][SceneUndo][Editor] Remove and move restore subtree owners and sibling order") {
	UndoRedo *undo = memnew(UndoRedo);
	Node *root = memnew(Node);
	Node *left = memnew(Node);
	Node *right = memnew(Node);
	Node *child = memnew(Node);
	Node *grandchild = memnew(Node);
	// Ownership is invalidated on exit from a live tree. A detached fixture
	// retains owners and does not model the editor's removal/reparent lifecycle.
	SceneTree::get_singleton()->get_root()->add_child(root);
	root->add_child(left);
	root->add_child(right);
	left->add_child(child);
	child->add_child(grandchild);
	left->set_owner(root);
	right->set_owner(root);
	child->set_owner(root);
	grandchild->set_owner(root);
	undo->create_action("Move");
	GoAgenticsToolBridge::record_move_node(undo, child, right, 0);
	undo->commit_action();
	CHECK(child->get_parent() == right);
	CHECK(child->get_owner() == root);
	CHECK(grandchild->get_owner() == root);
	CHECK(undo->undo());
	CHECK(child->get_parent() == left);
	CHECK(child->get_owner() == root);
	CHECK(grandchild->get_owner() == root);
	CHECK(undo->redo());
	undo->create_action("Remove");
	GoAgenticsToolBridge::record_remove_node(undo, child);
	undo->commit_action();
	CHECK(child->get_parent() == nullptr);
	CHECK(child->get_owner() == nullptr);
	CHECK(grandchild->get_owner() == nullptr);
	CHECK(undo->undo());
	CHECK(child->get_parent() == right);
	CHECK(child->get_owner() == root);
	CHECK(grandchild->get_owner() == root);
	CHECK(child->get_index() == 0);
	CHECK(undo->redo());
	// Clearing a performed removal frees the detached subtree via undo-reference.
	const ObjectID removed = child->get_instance_id();
	undo->clear_history();
	CHECK(ObjectDB::get_instance(removed) == nullptr);
	memdelete(root);
	memdelete(undo);
}

TEST_CASE("[GO.][SceneUndo][Editor] Scalar properties validate types and restore old values") {
	UndoRedo *undo = memnew(UndoRedo);
	Label *label = memnew(Label);
	label->set_text("Before");
	Variant value;
	CHECK(GoAgenticsToolBridge::validate_scalar_property(label, "text", "After", value).empty());
	undo->create_action("Property");
	GoAgenticsToolBridge::record_set_property(undo, label, "text", value);
	undo->commit_action();
	CHECK(label->get_text() == "After");
	CHECK(undo->undo());
	CHECK(label->get_text() == "Before");
	CHECK(undo->redo());
	CHECK(label->get_text() == "After");
	CHECK_FALSE(GoAgenticsToolBridge::validate_scalar_property(label, "text", 12, value).empty());
	CHECK_FALSE(GoAgenticsToolBridge::validate_scalar_property(label, "script", "res://evil.gd", value).empty());
	CHECK_FALSE(GoAgenticsToolBridge::validate_scalar_property(label, "visible", "true", value).empty());
	CHECK_FALSE(GoAgenticsToolBridge::validate_scalar_property(label, "z_index", 2.5, value).empty());
	CHECK_FALSE(GoAgenticsToolBridge::validate_scalar_property(label, "z_index", 4097, value).empty());
	Node2D *node = memnew(Node2D);
	CHECK(GoAgenticsToolBridge::validate_scalar_property(node, "rotation_degrees", 45.0, value).empty());
	CHECK_FALSE(GoAgenticsToolBridge::validate_scalar_property(node, "rotation_degrees", std::numeric_limits<double>::infinity(), value).empty());
	CHECK_FALSE(GoAgenticsToolBridge::validate_scalar_property(node, "text", "No such property", value).empty());
	undo->clear_history();
	memdelete(node);
	memdelete(label);
	memdelete(undo);
}

} // namespace TestGoSceneUndo
