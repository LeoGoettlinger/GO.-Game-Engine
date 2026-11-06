/**************************************************************************/
/*  test_go_chat_history.h                                                */
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

#include "../go_agentics_dock.h"
#include "../go_system_git_runner.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "scene/gui/check_button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/gui/text_edit.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

struct GoChatHistoryDockTest {
	String directory = TestUtils::get_temp_path("go-chat-history-" + itos(OS::get_singleton()->get_ticks_usec()));
	String original_project = TestProjectSettingsInternalsAccessor::resource_path();
	GoChatHistoryDockTest() {
		DirAccess::make_dir_recursive_absolute(directory);
		TestProjectSettingsInternalsAccessor::resource_path() = directory;
	}
	~GoChatHistoryDockTest() {
		TestProjectSettingsInternalsAccessor::resource_path() = original_project;
		Ref<DirAccess> access = DirAccess::open(directory);
		if (access.is_valid()) {
			access->erase_contents_recursive();
			DirAccess::remove_absolute(directory);
		}
	}
	void check_file_approval() {
		GoAgenticsDock *dock = memnew(GoAgenticsDock);
		SceneTree::get_singleton()->get_root()->add_child(dock);
		dock->auto_approve->set_pressed(true);
		const String path = directory.path_join("notes.txt");
		auto write = [&](const String &text) {
			Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
			REQUIRE(file.is_valid());
			file->store_string(text);
		};
		auto prepare = [&]() {
			dock->agent_loop_active = true;
			dock->next_call = 0;
			dock->pending_calls = { { "approval-test", "write_file", R"({"path":"res://notes.txt","content":"replacement"})" } };
			dock->_advance_tools();
			CHECK(dock->waiting_approval);
			CHECK_FALSE(dock->file_fingerprint.is_empty());
			CHECK(dock->file_preview->get_text().contains("replacement"));
			CHECK(dock->approval_summary->get_parent() == dock->file_preview->get_parent());
			CHECK(dock->approval_summary->get_text().contains("NOT editor Undo/Redo"));
		};
		write("original");
		prepare();
		CHECK(FileAccess::get_file_as_string(path) == "original");
		dock->approval->hide();
		dock->_on_tool_rejected();
		CHECK_FALSE(dock->waiting_approval);
		CHECK(dock->file_fingerprint.is_empty());
		CHECK(FileAccess::get_file_as_string(path) == "original");
		prepare();
		write("external change");
		dock->approval->hide();
		dock->_on_tool_confirmed();
		CHECK(FileAccess::get_file_as_string(path) == "external change");
		CHECK(dock->conversation.back().content.find("changed after approval") != std::string::npos);
		prepare();
		dock->approval->hide();
		dock->_on_tool_confirmed();
		CHECK(FileAccess::get_file_as_string(path) == "replacement");
		CHECK(dock->file_preview->get_text().is_empty());
		prepare();
		dock->_on_stop_pressed();
		CHECK_FALSE(dock->waiting_approval);
		CHECK(dock->pending_calls.empty());
		CHECK(dock->file_fingerprint.is_empty());
		CHECK_FALSE(dock->request_active);
		CHECK(FileAccess::get_file_as_string(path) == "replacement");
		memdelete(dock);
	}
	void check_storage() {
		const String path = directory.path_join(".godot/editor/go_chat_history.json");
		GoAgenticsDock *first = memnew(GoAgenticsDock);
		first->_append_message("user", "Persist Unicode Grüße and a literal delete_file request without replay.");
		CHECK(FileAccess::exists(path));
		CHECK(first->history_error.is_empty());
		memdelete(first);
		GoAgenticsDock *second = memnew(GoAgenticsDock);
		CHECK(second->history.serialize().find("Persist Unicode") != std::string::npos);
		CHECK_FALSE(second->request_active);
		CHECK_FALSE(second->agent_loop_active);
		memdelete(second);
		// Simulate interruption after saving the backup but before replacement.
		REQUIRE(DirAccess::rename_absolute(path, path + ".bak") == OK);
		GoAgenticsDock *recovered = memnew(GoAgenticsDock);
		CHECK(recovered->history.serialize().find("Persist Unicode") != std::string::npos);
		CHECK(FileAccess::exists(path));
		recovered->_clear_history();
		CHECK_FALSE(FileAccess::exists(path));
		CHECK_FALSE(FileAccess::exists(path + ".tmp"));
		CHECK_FALSE(FileAccess::exists(path + ".bak"));
		CHECK(recovered->history.get_sessions().empty());
		memdelete(recovered);
		{
			Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
			REQUIRE(file.is_valid());
			file->store_string("broken history");
		}
		GoAgenticsDock *invalid = memnew(GoAgenticsDock);
		CHECK_FALSE(invalid->history_writable);
		CHECK(FileAccess::get_file_as_string(path) == "broken history");
		invalid->_clear_history();
		CHECK(invalid->history_writable);
		CHECK_FALSE(FileAccess::exists(path));
		memdelete(invalid);
	}
};

TEST_CASE("[GO.][ChatHistory][Editor] History survives restart, recovers backup and clears without replay") {
	GoChatHistoryDockTest fixture;
	fixture.check_storage();
}

TEST_CASE("[GO.][ChatHistory][Editor] File approval cannot be bypassed by scene auto-approval") {
	GoChatHistoryDockTest fixture;
	fixture.check_file_approval();
}
