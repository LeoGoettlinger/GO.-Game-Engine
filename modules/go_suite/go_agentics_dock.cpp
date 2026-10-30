/**************************************************************************/
/*  go_agentics_dock.cpp                                                  */
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

#include "go_agentics_dock.h"

#include "go_agentics_tools.h"
#include "go_kilo_client.h"
#include "go_kilo_models.h"
#include "go_system_git_runner.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/os/keyboard.h"
#include "core/string/ustring.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/check_button.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/text_edit.h"

#include <sstream>

namespace {
constexpr int MAX_AGENT_LOOP_STEPS = 20;
} // namespace

void GoAgenticsDock::_append_message(const String &p_role, const String &p_text) {
	if (chat_display == nullptr) {
		return;
	}
	chat_display->add_text("[" + p_role + "] " + p_text);
	chat_display->add_newline();
}

void GoAgenticsDock::_update_busy_state() {
	if (send_button != nullptr) {
		send_button->set_disabled(request_active);
	}
	if (stop_button != nullptr) {
		stop_button->set_disabled(!request_active);
	}
	if (chat_input != nullptr) {
		chat_input->set_editable(!request_active);
	}
	if (refresh_models_button != nullptr) {
		refresh_models_button->set_disabled(request_active);
	}
	if (auto_approve != nullptr) {
		auto_approve->set_disabled(true);
	}
	if (model_picker != nullptr) {
		model_picker->set_disabled(request_active);
	}
}

void GoAgenticsDock::_send_agent_request() {
	if (kilo_client == nullptr || request_active) {
		return;
	}
	if (conversation_roles.is_empty() || conversation_roles[0] != "system") {
		const String system_prompt = TTR("You are Code Agentics inside GO. Game Engine. Use only the provided whitelisted tools. Every path must be res:// and stay inside the open project. Use native tool calls when available; otherwise reply with compact JSON containing tool and arguments.");
		conversation_roles.insert(0, "system");
		conversation_contents.insert(0, system_prompt);
	}
	request_active = true;
	_update_busy_state();
	kilo_client->request_chat(selected_model_id, conversation_roles, conversation_contents, false, String());
}

bool GoAgenticsDock::_process_tool_calls(const String &p_tool_calls_json) {
	(void)p_tool_calls_json;
	_append_message(TTR("System"), TTR("Tool execution is disabled until confirmation, undo and filesystem safety tests are complete."));
	return false;
}

void GoAgenticsDock::_on_send_pressed() {
	if (request_active || chat_input == nullptr || kilo_client == nullptr) {
		return;
	}
	const String user_text = chat_input->get_text().strip_edges();
	if (user_text.is_empty()) {
		return;
	}
	chat_input->set_text("");
	_append_message(TTR("You"), user_text);
	conversation_roles.push_back("user");
	conversation_contents.push_back(user_text);
	agent_loop_active = true;
	agent_loop_steps = 0;
	_send_agent_request();
}

void GoAgenticsDock::_on_stop_pressed() {
	agent_loop_active = false;
	if (kilo_client != nullptr) {
		kilo_client->cancel_request();
	} else {
		_on_request_cancelled();
	}
}

void GoAgenticsDock::_on_new_conversation_pressed() {
	if (kilo_client != nullptr && request_active) {
		kilo_client->cancel_request();
	}
	if (chat_display != nullptr) {
		chat_display->clear();
	}
	conversation_roles.clear();
	conversation_contents.clear();
	request_active = false;
	agent_loop_active = false;
	agent_loop_steps = 0;
	_update_busy_state();
	_append_message(TTR("System"), TTR("New conversation."));
}

void GoAgenticsDock::_on_history_pressed() {
	if (chat_display == nullptr) {
		return;
	}
	if (conversation_roles.is_empty()) {
		_append_message(TTR("System"), TTR("This conversation has no stored messages yet."));
		return;
	}
	for (int i = 0; i < conversation_roles.size(); ++i) {
		_append_message(TTR("History") + " " + conversation_roles[i], conversation_contents[i].substr(0, 500));
	}
}

void GoAgenticsDock::_on_about_tools_pressed() {
	_append_message(TTR("System"), String::utf8(go::GoAgenticsTools::tool_definitions_json().c_str()));
}

void GoAgenticsDock::_on_refresh_models_pressed() {
	if (request_active || kilo_client == nullptr) {
		return;
	}
	if (model_status != nullptr) {
		model_status->set_text(TTR("Model catalog: loading free models…"));
	}
	request_active = true;
	_update_busy_state();
	kilo_client->request_models();
}

void GoAgenticsDock::_on_model_selected(int p_index) {
	if (p_index >= 0 && p_index < available_models.size()) {
		selected_model_id = available_models[p_index];
	}
}

void GoAgenticsDock::_on_chat_input_gui_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventKey> key = p_event;
	if (key.is_null() || !key->is_pressed() || key->is_echo()) {
		return;
	}
	if ((key->get_keycode() == Key::ENTER || key->get_keycode() == Key::KP_ENTER) && !key->is_shift_pressed()) {
		_on_send_pressed();
		if (chat_input != nullptr) {
			chat_input->accept_event();
		}
	}
}

void GoAgenticsDock::_on_models_loaded(const PackedStringArray &p_model_ids) {
	request_active = false;
	_update_busy_state();
	available_models = p_model_ids;
	if (available_models.is_empty()) {
		available_models.push_back(String::utf8(go::DEFAULT_FREE_MODEL));
	}
	if (model_picker != nullptr) {
		const String previous = selected_model_id;
		model_picker->clear();
		for (const String &model_id : available_models) {
			model_picker->add_item(model_id);
		}
		int index = available_models.find(previous);
		if (index < 0) {
			index = 0;
		}
		model_picker->select(index);
		selected_model_id = available_models[index];
	}
	if (model_status != nullptr) {
		model_status->set_text(TTR("Model catalog: live free models"));
	}
	_append_message(TTR("System"), TTR("Free model catalog loaded from the Kilo Gateway."));
}

void GoAgenticsDock::_on_chat_completed(const String &p_content, const String &p_tool_calls_json, bool p_used_json_fallback) {
	request_active = false;
	const String content = p_content.is_empty() ? TTR("(empty response)") : p_content;
	_append_message(TTR("Assistant"), content);
	conversation_roles.push_back("assistant");
	conversation_contents.push_back(content);
	if (p_used_json_fallback) {
		_append_message(TTR("System"), TTR("The model used the JSON tool-call fallback."));
	}

	const bool has_tool_calls = !p_tool_calls_json.is_empty() && p_tool_calls_json != "[]";
	bool continue_loop = false;
	if (agent_loop_active && has_tool_calls) {
		if (auto_approve == nullptr || !auto_approve->is_pressed()) {
			_append_message(TTR("System"), TTR("Tool calls were returned, but execution remains disabled pending safety and Undo/Redo validation."));
			agent_loop_active = false;
		} else if (agent_loop_steps >= MAX_AGENT_LOOP_STEPS) {
			_append_message(TTR("System"), TTR("Agent loop stopped after 20 steps."));
			agent_loop_active = false;
		} else {
			continue_loop = _process_tool_calls(p_tool_calls_json);
			if (!continue_loop) {
				agent_loop_active = false;
			}
		}
	} else {
		agent_loop_active = false;
	}

	if (continue_loop && agent_loop_steps < MAX_AGENT_LOOP_STEPS) {
		_send_agent_request();
		return;
	}
	if (agent_loop_steps >= MAX_AGENT_LOOP_STEPS) {
		_append_message(TTR("System"), TTR("Agent loop stopped after 20 steps."));
	}
	agent_loop_active = false;
	_update_busy_state();
}

void GoAgenticsDock::_on_request_failed(const String &p_message) {
	request_active = false;
	agent_loop_active = false;
	_update_busy_state();
	if (model_status != nullptr && model_picker != nullptr && model_picker->get_item_count() <= 1) {
		model_status->set_text(TTR("Model catalog: offline fallback"));
	}
	_append_message(TTR("System"), TTR("Kilo request failed: ") + p_message);
}

void GoAgenticsDock::_on_request_cancelled() {
	request_active = false;
	agent_loop_active = false;
	_update_busy_state();
	_append_message(TTR("System"), TTR("Cancelled."));
}

void GoAgenticsDock::_bind_methods() {
}

GoAgenticsDock::GoAgenticsDock() {
	set_custom_minimum_size(Size2(320, 0) * EDSCALE);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);

	Label *title = memnew(Label);
	title->set_text(TTR("Code Agentics"));
	title->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	add_child(title);

	Label *workspace = memnew(Label);
	const String project_path = ProjectSettings::get_singleton() != nullptr ? ProjectSettings::get_singleton()->globalize_path("res://") : String("res://");
	workspace->set_text(TTR("Workspace: res:// (locked)") + " · " + project_path);
	workspace->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	add_child(workspace);

	HBoxContainer *top_row = memnew(HBoxContainer);
	add_child(top_row);

	Label *model_label = memnew(Label);
	model_label->set_text(TTR("Model:"));
	top_row->add_child(model_label);

	model_picker = memnew(OptionButton);
	model_picker->add_item(String::utf8(go::DEFAULT_FREE_MODEL));
	model_picker->select(0);
	model_picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	model_picker->connect("item_selected", callable_mp(this, &GoAgenticsDock::_on_model_selected));
	top_row->add_child(model_picker);
	available_models.push_back(String::utf8(go::DEFAULT_FREE_MODEL));

	refresh_models_button = memnew(Button);
	refresh_models_button->set_text(TTR("Refresh Models"));
	refresh_models_button->connect("pressed", callable_mp(this, &GoAgenticsDock::_on_refresh_models_pressed));
	top_row->add_child(refresh_models_button);

	auto_approve = memnew(CheckButton);
	auto_approve->set_text(TTR("Auto-Approve"));
	auto_approve->set_disabled(true);
	auto_approve->set_tooltip_text(TTR("Unavailable while tool execution is disabled."));
	top_row->add_child(auto_approve);

	model_status = memnew(Label);
	model_status->set_text(TTR("Model catalog: offline fallback"));
	model_status->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	add_child(model_status);

	chat_display = memnew(RichTextLabel);
	chat_display->set_selection_enabled(true);
	chat_display->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_display->set_custom_minimum_size(Size2(0, 180) * EDSCALE);
	add_child(chat_display);

	chat_input = memnew(TextEdit);
	chat_input->set_placeholder(TTR("Ask Code Agentics about this project…"));
	chat_input->set_custom_minimum_size(Size2(0, 80) * EDSCALE);
	chat_input->connect("gui_input", callable_mp(this, &GoAgenticsDock::_on_chat_input_gui_input));
	add_child(chat_input);

	HBoxContainer *button_row = memnew(HBoxContainer);
	add_child(button_row);

	send_button = memnew(Button);
	send_button->set_text(TTR("Send"));
	send_button->connect("pressed", callable_mp(this, &GoAgenticsDock::_on_send_pressed));
	button_row->add_child(send_button);

	stop_button = memnew(Button);
	stop_button->set_text(TTR("Stop"));
	stop_button->connect("pressed", callable_mp(this, &GoAgenticsDock::_on_stop_pressed));
	button_row->add_child(stop_button);

	new_conversation_button = memnew(Button);
	new_conversation_button->set_text(TTR("New Conversation"));
	new_conversation_button->connect("pressed", callable_mp(this, &GoAgenticsDock::_on_new_conversation_pressed));
	button_row->add_child(new_conversation_button);

	history_button = memnew(Button);
	history_button->set_text(TTR("History"));
	history_button->connect("pressed", callable_mp(this, &GoAgenticsDock::_on_history_pressed));
	button_row->add_child(history_button);

	about_tools_button = memnew(Button);
	about_tools_button->set_text(TTR("About Tools"));
	about_tools_button->connect("pressed", callable_mp(this, &GoAgenticsDock::_on_about_tools_pressed));
	button_row->add_child(about_tools_button);

	Label *privacy = memnew(Label);
	privacy->set_text(TTR("Free models only · anonymous Kilo Gateway · prompts may be logged"));
	privacy->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	add_child(privacy);

	kilo_client = memnew(GoKiloClient);
	kilo_client->connect("models_loaded", callable_mp(this, &GoAgenticsDock::_on_models_loaded));
	kilo_client->connect("chat_completed", callable_mp(this, &GoAgenticsDock::_on_chat_completed));
	kilo_client->connect("request_failed", callable_mp(this, &GoAgenticsDock::_on_request_failed));
	kilo_client->connect("request_cancelled", callable_mp(this, &GoAgenticsDock::_on_request_cancelled));
	add_child(kilo_client);

	_update_busy_state();
	_append_message(TTR("System"), TTR("Code Agentics is ready. No request is sent until you press Send or Refresh Models."));
}
