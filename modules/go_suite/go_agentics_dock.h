/**************************************************************************/
/*  go_agentics_dock.h                                                    */
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

#include "go_agentics_tool_bridge.h"

#include "core/input/input_event.h"
#include "core/string/ustring.h"
#include "core/variant/variant.h"
#include "scene/gui/box_container.h"

class Button;
class CheckButton;
class GoKiloClient;
class Label;
class OptionButton;
class RichTextLabel;
class TextEdit;

class GoAgenticsDock : public VBoxContainer {
	GDCLASS(GoAgenticsDock, VBoxContainer);

	OptionButton *model_picker = nullptr;
	RichTextLabel *chat_display = nullptr;
	TextEdit *chat_input = nullptr;
	CheckButton *auto_approve = nullptr;
	Button *send_button = nullptr;
	Button *stop_button = nullptr;
	Button *new_conversation_button = nullptr;
	Button *history_button = nullptr;
	Button *about_tools_button = nullptr;
	Button *refresh_models_button = nullptr;
	Label *model_status = nullptr;
	GoKiloClient *kilo_client = nullptr;
	GoAgenticsToolBridge tool_bridge;

	PackedStringArray available_models;
	Vector<String> conversation_roles;
	Vector<String> conversation_contents;
	String selected_model_id = "kilo-auto/free";
	bool request_active = false;
	bool agent_loop_active = false;
	int agent_loop_steps = 0;

	void _append_message(const String &p_role, const String &p_text);
	void _update_busy_state();
	void _send_agent_request();
	bool _process_tool_calls(const String &p_tool_calls_json);
	void _on_send_pressed();
	void _on_stop_pressed();
	void _on_new_conversation_pressed();
	void _on_history_pressed();
	void _on_about_tools_pressed();
	void _on_refresh_models_pressed();
	void _on_model_selected(int p_index);
	void _on_chat_input_gui_input(const Ref<InputEvent> &p_event);
	void _on_models_loaded(const PackedStringArray &p_model_ids);
	void _on_chat_completed(const String &p_content, const String &p_tool_calls_json, bool p_used_json_fallback);
	void _on_request_failed(const String &p_message);
	void _on_request_cancelled();

protected:
	static void _bind_methods();

public:
	GoAgenticsDock();
};
