/**************************************************************************/
/*  test_go_read_tools.h                                                  */
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
#include "../go_system_git_runner.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

namespace TestGoReadTools {
struct Fixture {
	String directory = TestUtils::get_temp_path("go-read-tools-" + itos(OS::get_singleton()->get_ticks_usec()));
	String outside = directory + "-outside.txt";
	String original_project = TestProjectSettingsInternalsAccessor::resource_path();
	Fixture() {
		DirAccess::make_dir_recursive_absolute(directory);
		TestProjectSettingsInternalsAccessor::resource_path() = directory;
	}
	~Fixture() {
		if (FileAccess::exists(outside)) {
			DirAccess::remove_absolute(outside);
		}
		TestProjectSettingsInternalsAccessor::resource_path() = original_project;
		Ref<DirAccess> access = DirAccess::open(directory);
		if (access.is_valid()) {
			access->erase_contents_recursive();
			DirAccess::remove_absolute(directory);
		}
	}
	void write(const String &p_name, const String &p_text) {
		Ref<FileAccess> file = FileAccess::open(directory.path_join(p_name), FileAccess::WRITE);
		REQUIRE(file.is_valid());
		file->store_string(p_text);
	}
	String call(const std::string &p_name, const std::string &p_args) {
		return GoAgenticsToolBridge().execute({ "read-test", p_name, p_args });
	}
};

TEST_CASE("[GO.][ReadTools] Bounded reads and search return only approved project text") {
	Fixture fixture;
	fixture.write("player.gd", "extends Node\nvar speed = 100\n# movement\n");
	const String read = fixture.call("read_file", R"({"path":"res://player.gd","start_line":2,"line_limit":1})");
	CHECK(read.contains("\"ok\":true"));
	CHECK(read.contains("var speed = 100"));
	CHECK_FALSE(read.contains("extends Node"));
	CHECK_FALSE(read.contains("movement"));
	const String search = fixture.call("search_text", R"({"query":"speed"})");
	CHECK(search.contains("player.gd:2:"));
	CHECK(fixture.call("list_files", "{}").contains("player.gd"));
	CHECK(fixture.call("read_file", R"({"path":"res://player.gd","line_limit":401})").contains("\"ok\":false"));
	CHECK(fixture.call("read_file", R"({"path":"res://player.gd","start_line":0})").contains("\"ok\":false"));
}

TEST_CASE("[GO.][ReadTools] Sensitive binary oversized and foreign paths are refused") {
	Fixture fixture;
	fixture.write("ordinary.gd", "var api_key = \"fixture_only\"\n");
	fixture.write("secret.txt", "fixture confidential material");
	fixture.write("large.txt", String("x").repeat(200001));
	fixture.write("image.png", "not a supported text extension");
	for (const char *path : { "res://ordinary.gd", "res://secret.txt", "res://large.txt", "res://image.png", "res://../outside.txt", "res://.GIT/config", "res://NUL.txt", "/etc/passwd" }) {
		const String result = fixture.call("read_file", "{\"path\":" + go::escape_json_string(path) + "}");
		CHECK(result.contains("\"ok\":false"));
		CHECK_FALSE(result.contains("fixture_only"));
		CHECK_FALSE(result.contains("confidential material"));
	}
	CHECK_FALSE(fixture.call("search_text", R"({"query":"fixture"})").contains("fixture_only"));
	CHECK_FALSE(fixture.call("list_files", "{}").contains("secret.txt"));
	{
		Ref<FileAccess> file = FileAccess::open(fixture.directory.path_join("binary.txt"), FileAccess::WRITE);
		REQUIRE(file.is_valid());
		file->store_8(0);
	}
	CHECK(fixture.call("read_file", R"({"path":"res://binary.txt"})").contains("\"ok\":false"));
}
#ifndef WINDOWS_ENABLED
TEST_CASE("[GO.][ReadTools] Symlinks cannot read outside the project or enter listings") {
	Fixture fixture;
	{
		Ref<FileAccess> file = FileAccess::open(fixture.outside, FileAccess::WRITE);
		REQUIRE(file.is_valid());
		file->store_string("outside fixture sentinel");
	}
	Ref<DirAccess> access = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	REQUIRE(access->create_link(fixture.outside, fixture.directory.path_join("link.txt")) == OK);
	const String result = fixture.call("read_file", R"({"path":"res://link.txt"})");
	CHECK(result.contains("\"ok\":false"));
	CHECK_FALSE(result.contains("outside fixture sentinel"));
	CHECK_FALSE(fixture.call("list_files", "{}").contains("link.txt"));
}
#endif

} // namespace TestGoReadTools

namespace TestGoFileChanges {
TEST_CASE("[GO.][ReadTools] Prepared file changes reject stale approvals and preserve originals") {
	TestGoReadTools::Fixture fixture;
	fixture.write("notes.txt", "original text");
	GoAgenticsToolBridge bridge;
	go::GoKiloToolCall call = { "file-test", "write_file", R"({"path":"res://notes.txt","content":"replacement text"})" };
	String preview;
	String fingerprint;
	REQUIRE(bridge.prepare_file_change(call, preview, fingerprint).is_empty());
	CHECK(preview.contains("original text"));
	CHECK(preview.contains("replacement text"));
	CHECK(bridge.execute(call).contains("\"ok\":false"));
	fixture.write("notes.txt", "external change");
	CHECK(bridge.execute_file_change(call, fingerprint).contains("\"ok\":false"));
	CHECK(FileAccess::get_file_as_string("res://notes.txt") == "external change");
	REQUIRE(bridge.prepare_file_change(call, preview, fingerprint).is_empty());
	CHECK(bridge.execute_file_change(call, fingerprint).contains("\"ok\":true"));
	CHECK(FileAccess::get_file_as_string("res://notes.txt") == "replacement text");
	Ref<DirAccess> backups = DirAccess::open("res://.godot/go_agent_backups");
	REQUIRE(backups.is_valid());
	REQUIRE(backups->list_dir_begin() == OK);
	bool recovered = false;
	for (String name = backups->get_next(); !name.is_empty(); name = backups->get_next()) {
		if (!backups->current_is_dir()) {
			recovered |= FileAccess::get_file_as_string("res://.godot/go_agent_backups/" + name) == "external change";
		}
	}
	backups->list_dir_end();
	CHECK(recovered);
}

TEST_CASE("[GO.][ReadTools] File plans deny ambiguous edits protected files and editor scripts") {
	TestGoReadTools::Fixture fixture;
	fixture.write("notes.txt", "aaa");
	GoAgenticsToolBridge bridge;
	String preview;
	String fingerprint;
	for (const auto &call : {
				 go::GoKiloToolCall{ "test", "edit_file", R"({"path":"res://notes.txt","old_text":"aa","new_text":"b"})" },
				 go::GoKiloToolCall{ "test", "write_file", R"({"path":"res://project.godot","content":"bad"})" },
				 go::GoKiloToolCall{ "test", "write_file", R"({"path":"res://addons/plugin.gd","content":"extends Node"})" },
				 go::GoKiloToolCall{ "test", "create_script", R"({"path":"res://tool.gd","content":"@tool\nextends Node"})" },
				 go::GoKiloToolCall{ "test", "create_script", R"({"path":"res://tool.gd","content":"extends EditorPlugin"})" } }) {
		CHECK_FALSE(bridge.prepare_file_change(call, preview, fingerprint).is_empty());
		CHECK(fingerprint.is_empty());
	}
	CHECK(FileAccess::get_file_as_string("res://notes.txt") == "aaa");
	go::GoKiloToolCall create = { "test", "create_script", R"({"path":"res://player.gd","content":"extends Node\nvar speed = 100\n"})" };
	REQUIRE(bridge.prepare_file_change(create, preview, fingerprint).is_empty());
	CHECK(bridge.execute_file_change(create, fingerprint).contains("\"ok\":true"));
	CHECK(FileAccess::get_file_as_string("res://player.gd").contains("var speed = 100"));
}
} // namespace TestGoFileChanges
