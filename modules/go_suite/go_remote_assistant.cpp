// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_remote_assistant.h"

#include "go_git_remote.h"
#include "go_system_git_runner.h"

#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"

go::GitHost GoRemoteAssistant::selected_host() const {
	if (host_picker == nullptr) {
		return go::GitHost::CUSTOM;
	}
	switch (host_picker->get_selected()) {
		case 0:
			return go::GitHost::GITHUB;
		case 1:
			return go::GitHost::CODEBERG;
		case 2:
			return go::GitHost::GITLAB;
		default:
			return go::GitHost::CUSTOM;
	}
}

std::string GoRemoteAssistant::selected_protocol() const {
	if (protocol_picker != nullptr && protocol_picker->get_selected() == 1) {
		return "ssh";
	}
	return "https";
}

std::string GoRemoteAssistant::build_url() const {
	if (owner_edit == nullptr || repo_edit == nullptr || host_edit == nullptr) {
		return "";
	}
	const std::string owner = go_suite::to_std_string(owner_edit->get_text().strip_edges());
	const std::string repo = go_suite::to_std_string(repo_edit->get_text().strip_edges());
	if (owner.empty() || repo.empty()) {
		return "";
	}
	const go::GitHost host = selected_host();
	if (host == go::GitHost::CUSTOM) {
		const std::string custom_host = go_suite::to_std_string(host_edit->get_text().strip_edges());
		if (custom_host.empty()) {
			return "";
		}
		return go::build_remote_url(custom_host, owner, repo, selected_protocol());
	}
	return go::build_remote_url(host, owner, repo, selected_protocol());
}

void GoRemoteAssistant::_on_host_selected(int p_index) {
	(void)p_index;
	if (host_edit != nullptr) {
		host_edit->set_editable(selected_host() == go::GitHost::CUSTOM);
	}
	if (create_button != nullptr) {
		create_button->set_disabled(go::host_create_url(selected_host()).empty());
	}
	_update_preview();
}

void GoRemoteAssistant::_on_protocol_selected(int p_index) {
	(void)p_index;
	_update_preview();
}

void GoRemoteAssistant::_on_fields_changed(const String &p_text) {
	(void)p_text;
	_update_preview();
}

void GoRemoteAssistant::_on_create_pressed() {
	const std::string create_url = go::host_create_url(selected_host());
	if (create_url.empty()) {
		if (url_preview != nullptr) {
			url_preview->set_text(TTR("Create the repository on your custom host first, then enter the URL data above."));
		}
		return;
	}
	if (OS::get_singleton() != nullptr) {
		OS::get_singleton()->shell_open(go_suite::to_godot_string(create_url));
	}
}

void GoRemoteAssistant::_on_confirmed() {
	const std::string url = build_url();
	if (url.empty()) {
		if (url_preview != nullptr) {
			url_preview->set_text(TTR("Enter a valid owner, repository and custom host first."));
		}
		return;
	}
	if (controller == nullptr) {
		if (url_preview != nullptr) {
			url_preview->set_text(TTR("Git controller is unavailable."));
		}
		return;
	}
	std::string error;
	if (!controller->set_remote(url, error)) {
		if (url_preview != nullptr) {
			url_preview->set_text(TTR("Could not set remote: ") + go_suite::to_godot_string(error));
		}
		return;
	}
	if (url_preview != nullptr) {
		url_preview->set_text(TTR("Remote updated: ") + go_suite::to_godot_string(url));
	}
	hide();
}

void GoRemoteAssistant::_update_preview() {
	const std::string url = build_url();
	if (url_preview == nullptr) {
		return;
	}
	if (url.empty()) {
		url_preview->set_text(TTR("Preview: enter owner and repository name."));
		get_ok_button()->set_disabled(true);
		return;
	}
	url_preview->set_text(TTR("Preview: ") + go_suite::to_godot_string(url));
	get_ok_button()->set_disabled(false);
}

void GoRemoteAssistant::_bind_methods() {
}

GoRemoteAssistant::GoRemoteAssistant(go_suite::GoGitController *p_controller) {
	controller = p_controller;
	set_title(TTR("GO. Git Remote Assistant"));
	set_min_size(Size2i(540, 300));
	set_hide_on_ok(false);
	set_ok_button_text(TTR("Set Remote"));

	VBoxContainer *root = memnew(VBoxContainer);
	root->set_custom_minimum_size(Size2(500, 0));
	add_child(root);

	Label *intro = memnew(Label);
	intro->set_text(TTR("Choose a Git host. GO. never stores passwords or access credentials; system Git handles authentication."));
	intro->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	root->add_child(intro);

	HBoxContainer *host_row = memnew(HBoxContainer);
	root->add_child(host_row);
	host_picker = memnew(OptionButton);
	host_picker->add_item(TTR("GitHub"));
	host_picker->add_item(TTR("Codeberg"));
	host_picker->add_item(TTR("GitLab"));
	host_picker->add_item(TTR("Custom host"));
	host_picker->select(0);
	host_picker->connect("item_selected", callable_mp(this, &GoRemoteAssistant::_on_host_selected));
	host_row->add_child(host_picker);

	protocol_picker = memnew(OptionButton);
	protocol_picker->add_item(TTR("HTTPS"));
	protocol_picker->add_item(TTR("SSH"));
	protocol_picker->select(0);
	protocol_picker->connect("item_selected", callable_mp(this, &GoRemoteAssistant::_on_protocol_selected));
	host_row->add_child(protocol_picker);

	host_edit = memnew(LineEdit);
	host_edit->set_placeholder(TTR("git.example.com"));
	host_edit->set_editable(false);
	host_edit->connect("text_changed", callable_mp(this, &GoRemoteAssistant::_on_fields_changed));
	root->add_child(host_edit);

	owner_edit = memnew(LineEdit);
	owner_edit->set_placeholder(TTR("Owner or organization"));
	owner_edit->connect("text_changed", callable_mp(this, &GoRemoteAssistant::_on_fields_changed));
	root->add_child(owner_edit);

	repo_edit = memnew(LineEdit);
	repo_edit->set_placeholder(TTR("Repository name"));
	repo_edit->connect("text_changed", callable_mp(this, &GoRemoteAssistant::_on_fields_changed));
	root->add_child(repo_edit);

	HBoxContainer *action_row = memnew(HBoxContainer);
	root->add_child(action_row);
	create_button = memnew(Button);
	create_button->set_text(TTR("Open create page"));
	create_button->connect("pressed", callable_mp(this, &GoRemoteAssistant::_on_create_pressed));
	action_row->add_child(create_button);

	url_preview = memnew(Label);
	url_preview->set_text(TTR("Preview: enter owner and repository name."));
	url_preview->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	root->add_child(url_preview);

	connect("confirmed", callable_mp(this, &GoRemoteAssistant::_on_confirmed));
	_update_preview();
}
