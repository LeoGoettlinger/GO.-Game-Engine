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
#include "scene/gui/label.h"
#include "scene/gui/text_edit.h"

void GoLfsDialog::_append(const String &p_text) {
	if (output != nullptr) {
		output->set_text(output->get_text() + p_text + "\n");
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

	output = memnew(TextEdit);
	output->set_editable(false);
	output->set_custom_minimum_size(Size2(0, 160));
	output->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(output);

	_refresh_state();
}
