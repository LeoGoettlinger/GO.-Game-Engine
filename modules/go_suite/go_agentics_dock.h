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
