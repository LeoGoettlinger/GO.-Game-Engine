/**************************************************************************/
/*  go_lfs_dialog.cpp                                                     */
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

// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_lfs_dialog.h"

#include "go_git_lfs.h"
#include "go_git_remote.h"
#include "go_suite_core.h"
#include "go_system_git_runner.h"

#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/text_edit.h"

void GoLfsDialog::_append(const String &p_text) {
	if (output_view != nullptr) {
		output_view->set_text(output_view->get_text() + p_text + "\n");
	}
}

void GoLfsDialog::_refresh_state() {
	if (state_label == nullptr || controller == nullptr) {
		return;
	}
	std::string error;
	const go_suite::GoGitSnapshot snapshot = controller->snapshot(error);
	if (!snapshot.is_repo) {
		state_label->set_text(TTR("Not a Git repository."));
		return;
	}
	String text = TTR("Git LFS installed: ") + String(snapshot.lfs_installed ? "yes" : "no");
	text += "\n" + TTR("Project LFS ready: ") + String(snapshot.lfs_ready ? "yes" : "no");
	text += "\n" + TTR("Tracked LFS files: ") + String::num_int64(snapshot.lfs.files.size());
	text += "\n" + TTR("Total LFS size: ") + go_suite::to_godot_string(go::human_size(snapshot.lfs.total_bytes));
	text += "\n" + TTR("Objects staged / pending push: ") + String::num_int64(snapshot.lfs_staged) + " / " + String::num_int64(snapshot.lfs_pending);
	if (!snapshot.large_file_warnings.empty()) {
		text += "\n" + TTR("Large-file warnings: ") + String::num_int64(snapshot.large_file_warnings.size());
	}
	state_label->set_text(text);
}

void GoLfsDialog::_on_check_pressed() {
	_refresh_state();
	_append(TTR("Checked Git LFS state."));
}

void GoLfsDialog::_on_install_pressed() {
	if (controller == nullptr) {
		return;
	}
	std::string platform = "linux";
	if (OS::get_singleton() != nullptr && OS::get_singleton()->get_name().to_lower().contains("windows")) {
		platform = "windows";
	}
	_append(TTR("Running the system package manager with admin confirmation. This can be cancelled by the OS."));
	std::string output;
	std::string error;
	if (controller->lfs_install_system(platform, output, error)) {
		_append(TTR("System Git LFS installation command finished."));
		_append(go_suite::to_godot_string(output));
	} else {
		_append(TTR("System Git LFS installation failed: ") + go_suite::to_godot_string(error));
	}
	_refresh_state();
}

void GoLfsDialog::_on_init_pressed() {
	if (controller == nullptr) {
		return;
	}
	std::string output;
	std::string error;
	if (controller->lfs_install_local(output, error)) {
		_append(TTR("Git LFS initialized for this project."));
	} else {
		_append(TTR("Git LFS local init failed: ") + go_suite::to_godot_string(error));
	}
	_refresh_state();
}

void GoLfsDialog::_on_rules_pressed() {
	if (controller == nullptr) {
		return;
	}
	std::string error;
	if (controller->ensure_lfs_attributes(error)) {
		_append(TTR("Managed .gitattributes rules are present."));
	} else {
		_append(TTR("Could not write .gitattributes: ") + go_suite::to_godot_string(error));
	}
	_refresh_state();
}

void GoLfsDialog::_on_track_pressed() {
	if (controller != nullptr && controller->lfs_policy().mode != go::LfsMode::FILE_TYPES) {
		_on_rules_pressed();
		return;
	}
	if (controller == nullptr) {
		return;
	}
	std::string error;
	const go_suite::GoGitSnapshot snapshot = controller->snapshot(error);
	if (snapshot.suggested_lfs_patterns.empty()) {
		_append(TTR("No additional LFS patterns are needed right now."));
		return;
	}
	std::string output;
	if (controller->lfs_track_patterns(snapshot.suggested_lfs_patterns, output, error)) {
		_append(TTR("Tracked suggested LFS patterns."));
		_append(go_suite::to_godot_string(go::join(snapshot.suggested_lfs_patterns, ", ")));
	} else {
		_append(TTR("Could not track LFS patterns: ") + go_suite::to_godot_string(error));
	}
	_refresh_state();
}

void GoLfsDialog::_on_pull_pressed() {
	if (controller == nullptr) {
		return;
	}
	std::string output;
	std::string error;
	if (controller->lfs_pull(output, error)) {
		_append(TTR("Git LFS objects downloaded."));
	} else {
		_append(TTR("Git LFS download failed: ") + go_suite::to_godot_string(error));
	}
	_refresh_state();
}

void GoLfsDialog::_on_push_pressed() {
	if (controller == nullptr) {
		return;
	}
	std::string output;
	std::string error;
	if (controller->lfs_push(output, error)) {
		_append(TTR("Git LFS objects uploaded."));
	} else {
		_append(TTR("Git LFS upload failed: ") + go_suite::to_godot_string(error));
	}
	_refresh_state();
}

void GoLfsDialog::_on_quota_pressed() {
	if (controller == nullptr) {
		return;
	}
	std::string error;
	const go_suite::GoGitSnapshot snapshot = controller->snapshot(error);
	_append(TTR("Largest tracked LFS files:"));
	for (const go::LfsFile &file : snapshot.lfs.largest) {
		_append(go_suite::to_godot_string(file.path + " — " + go::human_size(file.size_bytes)));
	}
	const std::string warning = go::quota_warning(snapshot.remote.host_kind, snapshot.lfs.total_bytes);
	if (!warning.empty()) {
		_append(TTR("Quota note: ") + go_suite::to_godot_string(warning));
	} else {
		_append(TTR("No host quota warning threshold reached."));
	}
}

void GoLfsDialog::_on_mode_selected(int p_index) {
	threshold->set_editable(p_index == 1);
	folder_list->set_visible(p_index == 2);
}

void GoLfsDialog::_on_add_folder() {
	folder_picker->popup_centered_ratio();
}

void GoLfsDialog::_on_folder_selected(const String &p_path) {
	if (!p_path.begins_with("res://")) {
		return;
	}
	String folder = p_path.trim_prefix("res://").trim_suffix("/");
	if (!go::valid_lfs_folder(go_suite::to_std_string(folder))) {
		_append(TTR("Choose a project subfolder, not the project root, a hidden folder or an export folder."));
		return;
	}
	for (int i = 0; i < folder_list->get_item_count(); ++i) {
		if (folder_list->get_item_text(i) == folder) {
			return;
		}
	}
	folder_list->add_item(folder);
	mode_picker->select(2);
	_on_mode_selected(2);
}

void GoLfsDialog::_on_remove_folder() {
	const PackedInt32Array selected = folder_list->get_selected_items();
	for (int i = selected.size() - 1; i >= 0; --i) {
		folder_list->remove_item(selected[i]);
	}
}

void GoLfsDialog::_on_save_policy() {
	if (controller == nullptr) {
		return;
	}
	go::LfsPolicy policy;
	policy.mode = static_cast<go::LfsMode>(mode_picker->get_selected());
	policy.threshold_mib = uint64_t(threshold->get_value());
	for (int i = 0; i < folder_list->get_item_count(); ++i) {
		policy.folders.push_back(go_suite::to_std_string(folder_list->get_item_text(i)));
	}
	std::string error;
	if (!controller->save_lfs_policy(policy, error)) {
		_append(TTR("Could not save LFS settings: ") + go_suite::to_godot_string(error));
		return;
	}
	_append(TTR("LFS settings saved locally. Write LFS rules to apply now; GO. also applies them before staging. Existing commits are unchanged."));
}

void GoLfsDialog::_bind_methods() {
}

GoLfsDialog::GoLfsDialog(go_suite::GoGitController *p_controller) {
	controller = p_controller;
	set_title(TTR("GO. Git LFS"));
	set_min_size(Size2i(620, 400));
	get_ok_button()->set_text(TTR("Close"));

	VBoxContainer *root = memnew(VBoxContainer);
	root->set_custom_minimum_size(Size2(580, 0));
	add_child(root);

	state_label = memnew(Label);
	state_label->set_text(TTR("Press Check to read the Git LFS state."));
	state_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	root->add_child(state_label);

	Label *policy_label = memnew(Label);
	policy_label->set_text(TTR("LFS selection (text formats stay in normal Git):"));
	root->add_child(policy_label);
	mode_picker = memnew(OptionButton);
	mode_picker->add_item(TTR("File types (legacy default)"));
	mode_picker->add_item(TTR("Automatic: files at or above size threshold"));
	mode_picker->add_item(TTR("Semi-automatic: selected folders recursively"));
	root->add_child(mode_picker);
	threshold = memnew(SpinBox);
	threshold->set_min(1);
	threshold->set_max(1048576);
	threshold->set_suffix(TTR("MiB"));
	root->add_child(threshold);
	folder_list = memnew(ItemList);
	folder_list->set_custom_minimum_size(Size2(0, 80));
	folder_list->set_select_mode(ItemList::SELECT_MULTI);
	root->add_child(folder_list);
	folder_picker = memnew(FileDialog);
	folder_picker->set_access(FileDialog::ACCESS_RESOURCES);
	folder_picker->set_file_mode(FileDialog::FILE_MODE_OPEN_DIR);
	folder_picker->connect("dir_selected", callable_mp(this, &GoLfsDialog::_on_folder_selected));
	add_child(folder_picker);
	HBoxContainer *policy_buttons = memnew(HBoxContainer);
	root->add_child(policy_buttons);
	Button *add_folder = memnew(Button);
	add_folder->set_text(TTR("Add folder…"));
	add_folder->connect("pressed", callable_mp(this, &GoLfsDialog::_on_add_folder));
	policy_buttons->add_child(add_folder);
	Button *remove_folder = memnew(Button);
	remove_folder->set_text(TTR("Remove selected folders"));
	remove_folder->connect("pressed", callable_mp(this, &GoLfsDialog::_on_remove_folder));
	policy_buttons->add_child(remove_folder);
	Button *save_policy = memnew(Button);
	save_policy->set_text(TTR("Save LFS settings"));
	save_policy->connect("pressed", callable_mp(this, &GoLfsDialog::_on_save_policy));
	policy_buttons->add_child(save_policy);
	const go::LfsPolicy policy = controller != nullptr ? controller->lfs_policy() : go::LfsPolicy();
	mode_picker->select(int(policy.mode));
	threshold->set_value(policy.threshold_mib);
	for (const std::string &folder : policy.folders) {
		folder_list->add_item(go_suite::to_godot_string(folder));
	}
	mode_picker->connect("item_selected", callable_mp(this, &GoLfsDialog::_on_mode_selected));
	_on_mode_selected(int(policy.mode));

	HBoxContainer *row_one = memnew(HBoxContainer);
	root->add_child(row_one);
	check_button = memnew(Button);
	check_button->set_text(TTR("Check"));
	check_button->connect("pressed", callable_mp(this, &GoLfsDialog::_on_check_pressed));
	row_one->add_child(check_button);

	install_button = memnew(Button);
	install_button->set_text(TTR("Install via package manager"));
	install_button->connect("pressed", callable_mp(this, &GoLfsDialog::_on_install_pressed));
	row_one->add_child(install_button);

	init_button = memnew(Button);
	init_button->set_text(TTR("Initialize in project"));
	init_button->connect("pressed", callable_mp(this, &GoLfsDialog::_on_init_pressed));
	row_one->add_child(init_button);

	HBoxContainer *row_two = memnew(HBoxContainer);
	root->add_child(row_two);
	rules_button = memnew(Button);
	rules_button->set_text(TTR("Write LFS rules"));
	rules_button->connect("pressed", callable_mp(this, &GoLfsDialog::_on_rules_pressed));
	row_two->add_child(rules_button);

	track_button = memnew(Button);
	track_button->set_text(TTR("Track large files"));
	track_button->connect("pressed", callable_mp(this, &GoLfsDialog::_on_track_pressed));
	row_two->add_child(track_button);

	quota_button = memnew(Button);
	quota_button->set_text(TTR("Quota"));
	quota_button->connect("pressed", callable_mp(this, &GoLfsDialog::_on_quota_pressed));
	row_two->add_child(quota_button);

	HBoxContainer *row_three = memnew(HBoxContainer);
	root->add_child(row_three);
	pull_button = memnew(Button);
	pull_button->set_text(TTR("Download objects"));
	pull_button->connect("pressed", callable_mp(this, &GoLfsDialog::_on_pull_pressed));
	row_three->add_child(pull_button);

	push_button = memnew(Button);
	push_button->set_text(TTR("Upload objects"));
	push_button->connect("pressed", callable_mp(this, &GoLfsDialog::_on_push_pressed));
	row_three->add_child(push_button);

	output_view = memnew(TextEdit);
	output_view->set_editable(false);
	output_view->set_custom_minimum_size(Size2(0, 160));
	output_view->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(output_view);

	_refresh_state();
}
