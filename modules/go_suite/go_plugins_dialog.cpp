/**************************************************************************/
/*  go_plugins_dialog.cpp                                                 */
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

#include "go_plugins_dialog.h"

#include "go_suite_registry.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_interface.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"

namespace {
bool safe_component(const String &p_text) {
	if (p_text.is_empty() || p_text == "." || p_text == "..") {
		return false;
	}
	for (int i = 0; i < p_text.length(); i++) {
		char32_t c = p_text[i];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
			return false;
		}
	}
	return true;
}
bool safe_tree(const String &p_path, int &r_count, int p_depth = 0) {
	if (p_depth > 32 || ++r_count > 20000) {
		return false;
	}
	Ref<DirAccess> parent = DirAccess::open(p_path.get_base_dir());
	if (parent.is_null() || parent->is_link(p_path.get_file())) {
		return false;
	}
	Ref<DirAccess> dir = DirAccess::open(p_path);
	if (dir.is_null() || dir->list_dir_begin() != OK) {
		return false;
	}
	while (true) {
		String name = dir->get_next();
		if (name.is_empty()) {
			break;
		}
		if (name == "." || name == "..") {
			continue;
		}
		if (dir->is_link(name) || ++r_count > 20000) {
			return false;
		}
		if (dir->current_is_dir() && !safe_tree(p_path.path_join(name), r_count, p_depth + 1)) {
			return false;
		}
	}
	return true;
}
} //namespace

bool GoPluginsDialog::is_safe_tree(const String &p_path) {
	String cursor = p_path.simplify_path();
	while (cursor != cursor.get_base_dir()) {
		Ref<DirAccess> parent = DirAccess::open(cursor.get_base_dir());
		if (parent.is_null() || parent->is_link(cursor.get_file())) {
			return false;
		}
		cursor = cursor.get_base_dir();
	}
	int count = 0;
	return safe_tree(p_path, count);
}

bool GoPluginsDialog::available() const {
	return !GoSuiteRegistry::is_git_job_busy() && !GoSuiteRegistry::is_editor_input_locked() && !GoSuiteRegistry::is_quit_pending() && EditorInterface::get_singleton() != nullptr && !EditorInterface::get_singleton()->is_playing_scene() && !EditorFileSystem::get_singleton()->is_scanning() && project == ProjectSettings::get_singleton()->globalize_path("res://");
}

void GoPluginsDialog::open() {
	project = ProjectSettings::get_singleton()->globalize_path("res://");
	bundle = OS::get_singleton()->get_executable_path().get_base_dir().path_join("go_bundled_plugins");
	if (!FileAccess::exists(bundle.path_join("plugins.json"))) {
		bundle = bundle.get_base_dir().get_base_dir().path_join("go_bundled_plugins");
	}
	catalog.clear();
	Ref<FileAccess> file = FileAccess::open(bundle.path_join("plugins.json"), FileAccess::READ);
	if (file.is_valid() && file->get_length() < 262144) {
		Variant parsed = JSON::parse_string(file->get_as_text());
		if (parsed.get_type() == Variant::DICTIONARY) {
			Dictionary data = parsed;
			Variant entries = data.get("plugins", Array());
			if (entries.get_type() == Variant::ARRAY) {
				catalog = entries;
			}
		}
	}
	refresh();
	popup_centered(Size2i(800, 540));
}

void GoPluginsDialog::refresh() {
	list->clear();
	for (int i = 0; i < catalog.size(); i++) {
		if (catalog[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary item = catalog[i];
		String id = item.get("id", "");
		String addon = item.get("addon_path", "");
		if (!safe_component(id) || !addon.begins_with("addons/") || !safe_component(addon.trim_prefix("addons/"))) {
			continue;
		}
		bool installed = FileAccess::exists(project.path_join(addon).path_join("plugin.cfg"));
		bool present = FileAccess::exists(bundle.path_join(id).path_join(addon).path_join("plugin.cfg"));
		bool enabled = installed && EditorInterface::get_singleton()->is_plugin_enabled(addon.trim_prefix("addons/"));
		String state = enabled ? TTR("Enabled") : installed ? TTR("Installed, disabled")
				: present									? TTR("Available offline")
															: TTR("Not bundled");
		int row = list->add_item(String(item.get("name", id)) + " — " + String(item.get("tag", "")) + " — " + state);
		list->set_item_metadata(row, i);
	}
	status->set_text(TTR("Offline catalog. Install never overwrites existing folders. Enabling executes third-party editor code; disabling keeps all files. No downloads or automatic updates."));
}

void GoPluginsDialog::request_action(int p_operation) {
	if (!available() || list->get_selected_items().is_empty()) {
		status->set_text(TTR("Select a plugin and wait for running operations to finish."));
		return;
	}
	pending = list->get_item_metadata(list->get_selected_items()[0]);
	operation = p_operation;
	Dictionary item = catalog[pending];
	approval->set_text(String(item.get("name", "")) + "\n" + String(item.get("notes", "")) + "\n\n" + (operation == 0 ? TTR("Copy this bundled addon into the project without overwriting existing files? Importing addon resources may execute editor code. Files are not removed on failure; inspect any reported staging folder.") : operation == 1 ? TTR("Enable this plugin and execute its third-party editor code? Conflicting enabled plugins must be disabled first. This is not undoable with scene Undo/Redo.")
																																																																																					   : TTR("Disable this plugin? Its files and project data are retained.")));
	approval->popup_centered(Size2i(640, 320));
}

void GoPluginsDialog::apply_action() {
	if (!available() || pending < 0 || pending >= catalog.size()) {
		status->set_text(TTR("Project changed or an operation is busy; reopen this dialog."));
		return;
	}
	Dictionary item = catalog[pending];
	String id = item.get("id", "");
	String addon = item.get("addon_path", "");
	String name = addon.trim_prefix("addons/");
	if (!safe_component(id) || !safe_component(name) || addon != "addons/" + name) {
		return;
	}
	Ref<DirAccess> project_dir = DirAccess::open(project);
	if (project_dir.is_null() || project_dir->is_link("addons")) {
		status->set_text(TTR("Refusing a linked addons folder."));
		return;
	}
	String destination = project.path_join(addon);
	if (operation == 0) {
		String source = bundle.path_join(id).path_join(addon);
		if (DirAccess::dir_exists_absolute(destination) || FileAccess::exists(destination) || !FileAccess::exists(source.path_join("plugin.cfg")) || !is_safe_tree(source)) {
			status->set_text(TTR("Install refused: destination exists, bundle is absent, or source is unsafe/too large."));
			return;
		}
		String staging = project.path_join("addons").path_join(".go-install-" + id);
		if (DirAccess::make_dir_recursive_absolute(project.path_join("addons")) != OK) {
			return;
		}
		Ref<DirAccess> addons = DirAccess::open(project.path_join("addons"));
		if (addons.is_null() || addons->is_link(name) || addons->is_link(staging.get_file()) || DirAccess::dir_exists_absolute(staging) || FileAccess::exists(staging)) {
			status->set_text(TTR("Install refused: destination or staging path exists/is linked."));
			return;
		}
		Ref<DirAccess> access = DirAccess::open(source);
		if (access->copy_dir(source, staging) != OK || DirAccess::rename_absolute(staging, destination) != OK) {
			status->set_text(TTR("Install failed. Inspect the retained staging directory:") + "\n" + staging);
			return;
		}
		EditorFileSystem::get_singleton()->scan();
	} else {
		if (!FileAccess::exists(destination.path_join("plugin.cfg")) || !is_safe_tree(destination)) {
			status->set_text(TTR("Installed addon is missing or contains unsafe paths."));
			return;
		}
		if (operation == 1) {
			Array conflicts = item.get("mutually_exclusive_with", Array());
			for (int i = 0; i < catalog.size(); i++) {
				Dictionary other = catalog[i];
				if (conflicts.has(other.get("id", "")) && EditorInterface::get_singleton()->is_plugin_enabled(String(other.get("addon_path", "")).trim_prefix("addons/"))) {
					status->set_text(TTR("Disable the conflicting dialogue plugin first."));
					return;
				}
			}
		}
		EditorInterface::get_singleton()->set_plugin_enabled(name, operation == 1);
	}
	refresh();
	status->set_text(TTR("Operation requested. Check editor output for import/startup errors; successful copying is not a compatibility guarantee."));
}

GoPluginsDialog::GoPluginsDialog() {
	set_title(TTR("GO. Bundled Plugins"));
	VBoxContainer *box = memnew(VBoxContainer);
	add_child(box);
	list = memnew(ItemList);
	list->set_custom_minimum_size(Size2(720, 300));
	box->add_child(list);
	HBoxContainer *actions = memnew(HBoxContainer);
	box->add_child(actions);
	const String labels[] = { TTR("Install offline"), TTR("Enable"), TTR("Disable") };
	for (int i = 0; i < 3; i++) {
		Button *button = memnew(Button);
		button->set_text(labels[i]);
		button->connect("pressed", callable_mp(this, &GoPluginsDialog::request_action).bind(i));
		actions->add_child(button);
	}
	status = memnew(Label);
	status->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	box->add_child(status);
	approval = memnew(ConfirmationDialog);
	add_child(approval);
	approval->connect("confirmed", callable_mp(this, &GoPluginsDialog::apply_action));
}
