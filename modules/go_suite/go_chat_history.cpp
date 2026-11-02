/**************************************************************************/
/*  go_chat_history.cpp                                                   */
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

#include "go_chat_history.h"

#include "go_kilo_models.h"

namespace go {

namespace {
std::string bounded_utf8(const std::string &p_text, size_t p_limit) {
	if (p_text.size() <= p_limit) {
		return p_text;
	}
	while (p_limit > 0 && (static_cast<unsigned char>(p_text[p_limit]) & 0xc0) == 0x80) {
		--p_limit;
	}
	return p_text.substr(0, p_limit);
}
} // namespace

void ChatHistory::new_session() {
	if (sessions.empty() || !sessions.back().empty()) {
		sessions.emplace_back();
	}
	if (sessions.size() > MAX_SESSIONS) {
		sessions.erase(sessions.begin());
	}
}

void ChatHistory::append(const std::string &p_role, const std::string &p_text) {
	if (sessions.empty()) {
		new_session();
	}
	sessions.back().push_back({ bounded_utf8(p_role, 80), bounded_utf8(p_text, MAX_MESSAGE_BYTES) });
	if (sessions.back().size() > MAX_MESSAGES) {
		sessions.back().erase(sessions.back().begin());
	}
	while (serialize().size() > MAX_BYTES) {
		if (sessions.size() > 1) {
			sessions.erase(sessions.begin());
		} else {
			sessions.back().erase(sessions.back().begin());
		}
	}
}

std::string ChatHistory::serialize() const {
	std::string result = "{\"version\":1,\"sessions\":[";
	bool first_session = true;
	for (const auto &session : sessions) {
		if (!first_session) {
			result += ',';
		}
		first_session = false;
		result += '[';
		bool first_message = true;
		for (const auto &message : session) {
			if (!first_message) {
				result += ',';
			}
			first_message = false;
			result += "{\"role\":" + escape_json_string(message.role) + ",\"text\":" + escape_json_string(message.text) + "}";
		}
		result += ']';
	}
	return result + "]}";
}

bool ChatHistory::load(const std::string &p_json, std::string &r_error) {
	r_error.clear();
	if (p_json.size() > MAX_BYTES) {
		r_error = "Chat history exceeds its size limit";
		return false;
	}
	const JsonValue document = parse_json(p_json, r_error);
	if (!r_error.empty()) {
		return false;
	}
	const JsonValue *version = document.get("version");
	const JsonValue *saved = document.get("sessions");
	if (version == nullptr || version->type != JsonValue::Type::NUMBER || version->as_number() != 1 || saved == nullptr || saved->type != JsonValue::Type::ARRAY || saved->array_value.size() > MAX_SESSIONS) {
		r_error = "Invalid chat history version or session list";
		return false;
	}
	std::vector<std::vector<Entry>> loaded;
	for (const auto &session : saved->array_value) {
		if (session.type != JsonValue::Type::ARRAY || session.array_value.size() > MAX_MESSAGES) {
			r_error = "Invalid chat history message list";
			return false;
		}
		loaded.emplace_back();
		for (const auto &message : session.array_value) {
			const JsonValue *role = message.get("role");
			const JsonValue *text = message.get("text");
			if (role == nullptr || text == nullptr || role->type != JsonValue::Type::STRING || text->type != JsonValue::Type::STRING || role->string_value.size() > 80 || text->string_value.size() > MAX_MESSAGE_BYTES) {
				r_error = "Invalid chat history entry";
				return false;
			}
			loaded.back().push_back({ role->string_value, text->string_value });
		}
	}
	sessions = std::move(loaded);
	return true;
}

} // namespace go
