// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "scene/gui/box_container.h"

class Button;
class CheckButton;
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

	void _append_message(const String &p_role, const String &p_text);
	void _on_send_pressed();
	void _on_stop_pressed();
	void _on_new_conversation_pressed();
	void _on_history_pressed();
	void _on_about_tools_pressed();

protected:
	static void _bind_methods();

public:
	GoAgenticsDock();
};
