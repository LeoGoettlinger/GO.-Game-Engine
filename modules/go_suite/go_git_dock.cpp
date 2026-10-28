/**************************************************************************/
/*  go_git_dock.cpp                                                       */
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

#include "go_git_dock.h"

#include "go_auto_commit_controller.h"
#include "go_git_remote.h"
#include "go_lfs_dialog.h"
#include "go_remote_assistant.h"
#include "go_suite_registry.h"
#include "go_system_git_runner.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/string/ustring.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/text_edit.h"

void GoGitDock::_append_output(const String &p_text) {
	if (output != nullptr) {
		output->set_text(output->get_text() + p_text + "\n");
	}
}

void GoGitDock::_on_refresh_pressed() {
	refresh_status();
}

void GoGitDock::_on_commit_pressed() {
	if (commit_input == nullptr || commit_input->get_text().strip_edges().is_empty()) {
		_append_output(TTR("Enter a commit message first."));
		return;
	}
	const std::string message = go_suite::to_std_string(commit_input->get_text().strip_edges());
	std::string output_text;
	std::string error;
	if (controller.commit(message, output_text, error)) {
		_append_output(TTR("Commit created."));
		if (!output_text.empty()) {
			_append_output(go_suite::to_godot_string(output_text));
		}
		commit_input->clear();
		refresh_status();
	} else {
		_append_output(TTR("Commit failed: ") + go_suite::to_godot_string(error));
	}
}

void GoGitDock::_on_pull_pressed() {
	std::string output_text;
	std::string error;
	if (controller.pull(output_text, error)) {
		_append_output(TTR("Pull finished."));
		if (!output_text.empty()) {
			_append_output(go_suite::to_godot_string(output_text));
		}
		refresh_status();
	} else {
		_append_output(TTR("Pull failed: ") + go_suite::to_godot_string(error));
	}
}

void GoGitDock::_on_push_pressed() {
	std::string output_text;
	std::string error;
	if (controller.push(false, output_text, error)) {
		_append_output(TTR("Push finished."));
		if (!output_text.empty()) {
			_append_output(go_suite::to_godot_string(output_text));
		}
		refresh_status();
	} else {
		_append_output(TTR("Push failed: ") + go_suite::to_godot_string(error));
	}
}

void GoGitDock::_on_auto_commit_pressed() {
	if (auto_commit_controller == nullptr || auto_commit_controller->is_running()) {
		return;
	}
	if (auto_commit_button != nullptr) {
		auto_commit_button->set_disabled(true);
	}
	if (!auto_commit_controller->start()) {
		_append_output(TTR("Automatic Commit could not start. Check the Git status and user configuration."));
		refresh_status();
	}
}

void GoGitDock::_on_auto_commit_finished(bool p_success, const String &p_message) {
	_append_output((p_success ? TTR("Success: ") : TTR("Error: ")) + p_message);
	refresh_status();
}

void GoGitDock::_on_remote_pressed() {
	if (remote_dialog != nullptr) {
		remote_dialog->popup_centered();
	}
}

void GoGitDock::_on_lfs_pressed() {
	if (lfs_dialog != nullptr) {
		lfs_dialog->popup_centered();
	}
}

void GoGitDock::_on_log_pressed() {
	std::string error;
	const go_suite::GoGitSnapshot snapshot = controller.snapshot(error);
	if (!snapshot.is_repo) {
		_append_output(TTR("No Git repository in the open project."));
		return;
	}
	for (const go::GitLogEntry &entry : snapshot.recent_log) {
		_append_output(go_suite::to_godot_string(entry.hash.substr(0, 8) + " " + entry.date + " " + entry.subject));
	}
}

void GoGitDock::_on_plugins_pressed() {
	_append_output(TTR("Bundled Plugins management is added in a later step."));
}

void GoGitDock::set_host_text(const String &p_text) {
	if (host_label != nullptr) {
		host_label->set_text(p_text);
	}
}

void GoGitDock::refresh_status() {
	std::string error;
	const go_suite::GoGitSnapshot snapshot = controller.snapshot(error);
	if (output != nullptr) {
		output->set_text("");
	}

	if (!snapshot.is_repo) {
		set_host_text(TTR("Host: Only local"));
		if (state_label != nullptr) {
			state_label->set_text(TTR("No Git repository in the open project."));
		}
		if (changes_list != nullptr) {
			changes_list->clear();
			changes_list->add_item(TTR("Initialize Git or open a project with a repository."));
		}
		for (Button *button : { commit_button, pull_button, push_button, remote_button, lfs_button, log_button }) {
			if (button != nullptr) {
				button->set_disabled(true);
			}
		}
		_append_output(TTR("Refresh: ") + go_suite::to_godot_string(error));
		return;
	}

	set_host_text(TTR("Host: ") + go_suite::to_godot_string(go::host_display(snapshot.remote.host_kind)));
	String state = TTR("Branch: ") + go_suite::to_godot_string(snapshot.branch);
	state += TTR("  Ahead/behind: ") + String::num_int64(snapshot.status.ahead) + " / " + String::num_int64(snapshot.status.behind);
	if (snapshot.user_name.empty() || snapshot.user_email.empty()) {
		state += "\n" + TTR("Warning: Git user.name or user.email is not configured.");
	}
	if (snapshot.status.merge_conflict) {
		state += "\n" + TTR("Warning: merge conflicts detected.");
	}
	if (!snapshot.lfs_installed) {
		state += "\n" + TTR("Git LFS is not installed; large assets may be committed as blobs.");
	}
	if (state_label != nullptr) {
		state_label->set_text(state);
	}

	if (changes_list != nullptr) {
		changes_list->clear();
		if (snapshot.status.entries.empty()) {
			changes_list->add_item(TTR("Working tree clean."));
		} else {
			for (const go::GitStatusEntry &entry : snapshot.status.entries) {
				String line = String::chr(entry.index_status) + String::chr(entry.worktree_status) + " " + go_suite::to_godot_string(entry.path);
				if (entry.conflict) {
					line += " " + TTR("(conflict)");
				}
				changes_list->add_item(line);
			}
		}
		for (const std::string &warning : snapshot.large_file_warnings) {
			changes_list->add_item(TTR("Large file: ") + go_suite::to_godot_string(warning));
		}
	}

	for (Button *button : { commit_button, pull_button, push_button, remote_button, lfs_button, log_button }) {
		if (button != nullptr) {
			button->set_disabled(false);
		}
	}
	_append_output(TTR("Status refreshed."));
}

void GoGitDock::_bind_methods() {
}

GoGitDock::GoGitDock() {
	if (ProjectSettings::get_singleton() != nullptr) {
		controller.set_project(ProjectSettings::get_singleton()->get_resource_path());
	}

	set_custom_minimum_size(Size2(320, 0) * EDSCALE);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);

	remote_dialog = memnew(GoRemoteAssistant(&controller));
	add_child(remote_dialog);
	lfs_dialog = memnew(GoLfsDialog(&controller));
	add_child(lfs_dialog);

	auto_commit_controller = memnew(GoAutoCommitController);
	auto_commit_controller->set_git_controller(&controller);
	auto_commit_controller->connect("finished", callable_mp(this, &GoGitDock::_on_auto_commit_finished));
	add_child(auto_commit_controller);
	// GO. Keep Automatic Commit disabled until worker-thread and UI-lock tests pass.

	Label *title = memnew(Label);
	title->set_text(TTR("GO. Git Suite"));
	title->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	add_child(title);

	host_label = memnew(Label);
	host_label->set_text(TTR("Host: Only local"));
	host_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	add_child(host_label);

	HBoxContainer *toolbar = memnew(HBoxContainer);
	add_child(toolbar);

	refresh_button = memnew(Button);
	refresh_button->set_text(TTR("Refresh"));
	refresh_button->connect("pressed", callable_mp(this, &GoGitDock::_on_refresh_pressed));
	toolbar->add_child(refresh_button);

	commit_button = memnew(Button);
	commit_button->set_text(TTR("Commit"));
	commit_button->set_disabled(true);
	commit_button->connect("pressed", callable_mp(this, &GoGitDock::_on_commit_pressed));
	toolbar->add_child(commit_button);

	pull_button = memnew(Button);
	pull_button->set_text(TTR("Pull"));
	pull_button->set_disabled(true);
	pull_button->connect("pressed", callable_mp(this, &GoGitDock::_on_pull_pressed));
	toolbar->add_child(pull_button);

	push_button = memnew(Button);
	push_button->set_text(TTR("Push"));
	push_button->set_disabled(true);
	push_button->connect("pressed", callable_mp(this, &GoGitDock::_on_push_pressed));
	toolbar->add_child(push_button);

	HBoxContainer *toolbar_secondary = memnew(HBoxContainer);
	add_child(toolbar_secondary);

	auto_commit_button = memnew(Button);
	auto_commit_button->set_text(TTR("Automatic Commit"));
	auto_commit_button->set_disabled(true);
	auto_commit_button->connect("pressed", callable_mp(this, &GoGitDock::_on_auto_commit_pressed));
	toolbar_secondary->add_child(auto_commit_button);

	remote_button = memnew(Button);
	remote_button->set_text(TTR("Remote Assistant"));
	remote_button->set_disabled(true);
	remote_button->connect("pressed", callable_mp(this, &GoGitDock::_on_remote_pressed));
	toolbar_secondary->add_child(remote_button);

	lfs_button = memnew(Button);
	lfs_button->set_text(TTR("Git LFS"));
	lfs_button->set_disabled(true);
	lfs_button->connect("pressed", callable_mp(this, &GoGitDock::_on_lfs_pressed));
	toolbar_secondary->add_child(lfs_button);

	HBoxContainer *toolbar_tertiary = memnew(HBoxContainer);
	add_child(toolbar_tertiary);

	log_button = memnew(Button);
	log_button->set_text(TTR("Log"));
	log_button->set_disabled(true);
	log_button->connect("pressed", callable_mp(this, &GoGitDock::_on_log_pressed));
	toolbar_tertiary->add_child(log_button);

	plugins_button = memnew(Button);
	plugins_button->set_text(TTR("Bundled Plugins"));
	plugins_button->set_disabled(true);
	plugins_button->connect("pressed", callable_mp(this, &GoGitDock::_on_plugins_pressed));
	toolbar_tertiary->add_child(plugins_button);

	commit_input = memnew(LineEdit);
	commit_input->set_placeholder(TTR("Commit message"));
	add_child(commit_input);

	state_label = memnew(Label);
	state_label->set_text(TTR("Press Refresh to read the Git status."));
	state_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	add_child(state_label);

	changes_list = memnew(ItemList);
	changes_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	changes_list->add_item(TTR("No repository status loaded yet."));
	add_child(changes_list);

	output = memnew(TextEdit);
	output->set_editable(false);
	output->set_custom_minimum_size(Size2(0, 120) * EDSCALE);
	output->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(output);
}
