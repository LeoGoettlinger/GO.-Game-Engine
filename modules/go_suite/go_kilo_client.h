/**************************************************************************/
/*  go_kilo_client.h                                                      */
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

#include "go_kilo_models.h"

#include "scene/main/node.h"

#include <string>
#include <vector>

class HTTPRequest;

class GoKiloClient : public Node {
	GDCLASS(GoKiloClient, Node);

public:
	enum class RequestKind {
		NONE,
		MODELS,
		CHAT,
	};

private:
	HTTPRequest *http = nullptr;
	RequestKind request_kind = RequestKind::NONE;
	std::vector<go::GoKiloModel> allowed_models = go::free_models_for_picker({});

	void _on_request_completed(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body);
	void _fail(const String &p_message);
	bool _begin_request(RequestKind p_kind, const String &p_url, const String &p_method, const String &p_body);
	Vector<String> _headers() const;

protected:
	static void _bind_methods();

public:
	String gateway_url() const;
	String models_url() const;
	String chat_url() const;
	String anonymous_key() const;
	String user_agent() const;
	std::vector<go::GoKiloModel> offline_models() const;

	void request_models();
	void request_chat(const String &p_model, const Vector<String> &p_roles, const Vector<String> &p_contents, bool p_tools_enabled, const String &p_tool_definitions_json);
	void request_conversation(const String &p_model, const std::vector<go::GoKiloMessage> &p_messages, bool p_tools_enabled, const String &p_tool_definitions_json);
	void request_chat_simple(const String &p_model, const String &p_prompt, const String &p_system_prompt = String());
	void cancel_request();
	bool is_busy() const { return request_kind != RequestKind::NONE; }

	GoKiloClient();
};
