/**************************************************************************/
/*  go_kilo_client.cpp                                                    */
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

#include "go_kilo_client.h"

#include "go_system_git_runner.h"

#include "core/io/http_client.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/string/ustring.h"
#include "scene/main/http_request.h"

#include <string>
#include <utility>
#include <vector>

void GoKiloClient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("request_models"), &GoKiloClient::request_models);
	ClassDB::bind_method(D_METHOD("request_chat", "model", "roles", "contents", "tools_enabled", "tool_definitions_json"), &GoKiloClient::request_chat);
	ClassDB::bind_method(D_METHOD("request_chat_simple", "model", "prompt", "system_prompt"), &GoKiloClient::request_chat_simple);
	ClassDB::bind_method(D_METHOD("cancel_request"), &GoKiloClient::cancel_request);
	ClassDB::bind_method(D_METHOD("is_busy"), &GoKiloClient::is_busy);

	ADD_SIGNAL(MethodInfo("models_loaded", PropertyInfo(Variant::PACKED_STRING_ARRAY, "model_ids")));
	ADD_SIGNAL(MethodInfo("chat_completed", PropertyInfo(Variant::STRING, "content"), PropertyInfo(Variant::STRING, "tool_calls_json"), PropertyInfo(Variant::BOOL, "used_json_fallback")));
	ADD_SIGNAL(MethodInfo("request_failed", PropertyInfo(Variant::STRING, "message")));
	ADD_SIGNAL(MethodInfo("request_cancelled"));
}

String GoKiloClient::gateway_url() const {
	return String::utf8(go::DEFAULT_KILO_GATEWAY_URL);
}

String GoKiloClient::models_url() const {
	return String::utf8(go::models_endpoint(go::DEFAULT_KILO_GATEWAY_URL).c_str());
}

String GoKiloClient::chat_url() const {
	return String::utf8(go::chat_endpoint(go::DEFAULT_KILO_GATEWAY_URL).c_str());
}

String GoKiloClient::anonymous_key() const {
	return String::utf8(go::ANONYMOUS_API_KEY);
}

String GoKiloClient::user_agent() const {
	return String::utf8(go::USER_AGENT_BASE) + "/GO.-Game-Engine";
}

std::vector<go::GoKiloModel> GoKiloClient::offline_models() const {
	return go::free_models_for_picker({});
}

Vector<String> GoKiloClient::_headers() const {
	Vector<String> headers;
	headers.push_back("Authorization: Bearer " + anonymous_key());
	headers.push_back("Content-Type: application/json");
	headers.push_back("User-Agent: " + user_agent());
	headers.push_back(String::utf8(go::EDITOR_HEADER_NAME) + ": " + String::utf8(go::EDITOR_HEADER_VALUE));
	return headers;
}

void GoKiloClient::_fail(const String &p_message) {
	request_kind = RequestKind::NONE;
	emit_signal("request_failed", p_message);
}

bool GoKiloClient::_begin_request(RequestKind p_kind, const String &p_url, const String &p_method, const String &p_body) {
	if (http == nullptr) {
		_fail(TTR("The Kilo HTTP client is not available."));
		return false;
	}
	if (is_busy()) {
		emit_signal("request_failed", TTR("A Kilo request is already running."));
		return false;
	}
	request_kind = p_kind;
	const Error error = http->request(p_url, _headers(), p_method == "POST" ? HTTPClient::METHOD_POST : HTTPClient::METHOD_GET, p_body);
	if (error != OK) {
		_fail(TTR("Could not start the Kilo Gateway request."));
		return false;
	}
	return true;
}

void GoKiloClient::request_models() {
	_begin_request(RequestKind::MODELS, models_url(), "GET", String());
}

void GoKiloClient::request_chat(const String &p_model, const Vector<String> &p_roles, const Vector<String> &p_contents, bool p_tools_enabled, const String &p_tool_definitions_json) {
	if (is_busy()) {
		emit_signal("request_failed", TTR("A Kilo request is already running."));
		return;
	}
	if (p_roles.size() != p_contents.size() || p_roles.is_empty()) {
		_fail(TTR("The chat request has no messages."));
		return;
	}
	std::vector<go::GoKiloMessage> messages;
	messages.reserve(p_roles.size());
	for (int i = 0; i < p_roles.size(); ++i) {
		std::string role = go_suite::to_std_string(p_roles[i]);
		if (role != "system" && role != "user" && role != "assistant") {
			_fail(TTR("Native tool messages require a matching tool call ID."));
			return;
		}
		messages.push_back({ role, go_suite::to_std_string(p_contents[i]), {}, "" });
	}
	request_conversation(p_model, messages, p_tools_enabled, p_tool_definitions_json);
}

void GoKiloClient::request_conversation(const String &p_model, const std::vector<go::GoKiloMessage> &p_messages, bool p_tools_enabled, const String &p_tool_definitions_json) {
	if (is_busy()) {
		emit_signal("request_failed", TTR("A Kilo request is already running."));
		return;
	}
	const std::string model = go_suite::to_std_string(p_model);
	bool allowed = false;
	for (const go::GoKiloModel &candidate : allowed_models) {
		allowed = allowed || (candidate.id == model && go::is_free_model(candidate));
	}
	if (!allowed) {
		_fail(TTR("Only free Kilo models are allowed in GO. Game Engine."));
		return;
	}
	if (p_tools_enabled) {
		bool supported = false;
		for (const go::GoKiloModel &candidate : allowed_models) {
			supported = supported || (candidate.id == model && candidate.supports_tools);
		}
		if (!supported) {
			_fail(TTR("The selected model has no verified native tool support."));
			return;
		}
	}
	std::string error;
	const std::string body = go::build_conversation_request_json(model, p_messages, p_tools_enabled, go_suite::to_std_string(p_tool_definitions_json), error);
	if (!error.empty()) {
		_fail(TTR("Invalid chat conversation: ") + go_suite::to_godot_string(error));
		return;
	}
	_begin_request(RequestKind::CHAT, chat_url(), "POST", go_suite::to_godot_string(body));
}

void GoKiloClient::request_chat_simple(const String &p_model, const String &p_prompt, const String &p_system_prompt) {
	Vector<String> roles;
	Vector<String> contents;
	if (!p_system_prompt.is_empty()) {
		roles.push_back("system");
		contents.push_back(p_system_prompt);
	}
	roles.push_back("user");
	contents.push_back(p_prompt);
	request_chat(p_model, roles, contents, false, String());
}

void GoKiloClient::cancel_request() {
	if (!is_busy()) {
		return;
	}
	if (http != nullptr) {
		http->cancel_request();
	}
	request_kind = RequestKind::NONE;
	emit_signal("request_cancelled");
}

void GoKiloClient::_on_request_completed(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body) {
	if (!is_busy()) {
		return;
	}
	const RequestKind completed_kind = request_kind;
	request_kind = RequestKind::NONE;

	if (p_result != HTTPRequest::RESULT_SUCCESS) {
		_fail(TTR("Could not reach the Kilo Gateway. Check your network connection."));
		return;
	}

	std::vector<std::string> headers;
	headers.reserve(p_headers.size());
	for (const String &header : p_headers) {
		headers.push_back(go_suite::to_std_string(header));
	}
	const std::string body = go_suite::to_std_string(String::utf8((const char *)p_body.ptr(), (int)p_body.size()));

	if (p_code < 200 || p_code >= 300) {
		const go::KiloHttpError http_error = go::parse_http_error(p_code, headers, body);
		String message = TTR("Kilo Gateway error: ") + go_suite::to_godot_string(http_error.message);
		if (http_error.is_rate_limit && http_error.retry_after_seconds > 0) {
			message += " " + vformat(TTR("Try again in %d seconds."), http_error.retry_after_seconds);
		}
		_fail(message);
		return;
	}

	if (completed_kind == RequestKind::MODELS) {
		std::string error;
		const std::vector<go::GoKiloModel> models = go::parse_models_response(body, error);
		if (!error.empty()) {
			_fail(TTR("Could not read the Kilo model catalog: ") + go_suite::to_godot_string(error));
			return;
		}
		allowed_models = go::free_models_for_picker(models);
		PackedStringArray model_ids;
		for (const go::GoKiloModel &model : go::free_models_for_picker(models)) {
			model_ids.push_back(go_suite::to_godot_string(model.id));
		}
		emit_signal("models_loaded", model_ids);
		return;
	}

	if (completed_kind == RequestKind::CHAT) {
		std::string error;
		const go::GoKiloChatResponse chat = go::parse_chat_response(body, error);
		if (!error.empty() || !chat.error.empty()) {
			_fail(TTR("Could not read the Kilo chat response: ") + go_suite::to_godot_string(error + chat.error));
			return;
		}
		emit_signal("chat_completed",
				go_suite::to_godot_string(chat.content),
				go_suite::to_godot_string(chat.tool_calls_json),
				chat.used_json_fallback);
	}
}

GoKiloClient::GoKiloClient() {
	http = memnew(HTTPRequest);
	http->set_use_threads(true);
	http->set_accept_gzip(true);
	http->set_timeout(30.0);
	http->set_body_size_limit(4 * 1024 * 1024);
	http->set_max_redirects(0);
	http->connect("request_completed", callable_mp(this, &GoKiloClient::_on_request_completed));
	add_child(http);
}
