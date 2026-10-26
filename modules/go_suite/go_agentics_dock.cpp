// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_agentics_dock.h"

#include "go_agentics_tools.h"
#include "go_kilo_models.h"

#include "core/object/callable_mp.h"
#include "core/string/ustring.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/check_button.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/text_edit.h"

void GoAgenticsDock::_append_message(const String &p_role, const String &p_text) {
	if (chat_display == nullptr) {
		return;
	}
	chat_display->add_text("[" + p_role + "] " + p_text);
	chat_display->add_newline();
}

void GoAgenticsDock::_on_send_pressed() {
	if (chat_input == nullptr || chat_input->get_text().strip_edges().is_empty()) {
		return;
	}
	const String user_text = chat_input->get_text().strip_edges();
	chat_input->set_text("");
	_append_message(TTR("You"), user_text);
	_append_message(TTR("System"), TTR("The Kilo Gateway client is not connected in this framework step."));
}

void GoAgenticsDock::_on_stop_pressed() {
	_append_message(TTR("System"), TTR("Stopped."));
}

void GoAgenticsDock::_on_new_conversation_pressed() {
	if (chat_display != nullptr) {
		chat_display->clear();
	}
	_append_message(TTR("System"), TTR("New conversation."));
}

void GoAgenticsDock::_on_history_pressed() {
	_append_message(TTR("System"), TTR("Conversation history is not wired yet."));
}

void GoAgenticsDock::_on_about_tools_pressed() {
	_append_message(TTR("System"), String::utf8(go::GoAgenticsTools::tool_definitions_json().c_str()));
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
	workspace->set_text(TTR("Workspace: res:// (locked)"));
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
	top_row->add_child(model_picker);

	auto_approve = memnew(CheckButton);
	auto_approve->set_text(TTR("Auto-Approve"));
	top_row->add_child(auto_approve);

	chat_display = memnew(RichTextLabel);
	chat_display->set_selection_enabled(true);
	chat_display->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	chat_display->set_custom_minimum_size(Size2(0, 180) * EDSCALE);
	add_child(chat_display);

	chat_input = memnew(TextEdit);
	chat_input->set_placeholder(TTR("Ask Code Agentics about this project…"));
	chat_input->set_custom_minimum_size(Size2(0, 80) * EDSCALE);
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

	_append_message(TTR("System"), TTR("Code Agentics framework is loaded. No request is sent until you press Send."));
}
