// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_git_dock.h"

#include "core/object/callable_mp.h"
#include "core/string/ustring.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
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
	_append_output(TTR("Commit is not wired to the system Git runner yet."));
}

void GoGitDock::_on_pull_pressed() {
	_append_output(TTR("Pull is not wired to the system Git runner yet."));
}

void GoGitDock::_on_push_pressed() {
	_append_output(TTR("Push is not wired to the system Git runner yet."));
}

void GoGitDock::_on_auto_commit_pressed() {
	_append_output(TTR("Automatic Commit is not wired to the overlay runner yet."));
}

void GoGitDock::_on_remote_pressed() {
	_append_output(TTR("Remote Assistant is not wired yet."));
}

void GoGitDock::_on_plugins_pressed() {
	_append_output(TTR("Bundled Plugins dialog is not wired yet."));
}

void GoGitDock::set_host_text(const String &p_text) {
	if (host_label != nullptr) {
		host_label->set_text(p_text);
	}
}

void GoGitDock::refresh_status() {
	if (state_label != nullptr) {
		state_label->set_text(TTR("GO. Git Suite framework is loaded. System Git operations are added in the Git Suite step."));
	}
	if (changes_list != nullptr) {
		changes_list->clear();
		changes_list->add_item(TTR("No repository status loaded yet."));
	}
	_append_output(TTR("Refresh: Git status runner not connected yet."));
}

void GoGitDock::_bind_methods() {
}

GoGitDock::GoGitDock() {
	set_custom_minimum_size(Size2(320, 0) * EDSCALE);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);

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

	plugins_button = memnew(Button);
	plugins_button->set_text(TTR("Bundled Plugins"));
	plugins_button->set_disabled(true);
	plugins_button->connect("pressed", callable_mp(this, &GoGitDock::_on_plugins_pressed));
	toolbar_secondary->add_child(plugins_button);

	state_label = memnew(Label);
	state_label->set_text(TTR("Git Suite framework ready."));
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
