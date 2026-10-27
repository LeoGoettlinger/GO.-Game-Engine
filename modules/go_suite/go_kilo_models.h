/**************************************************************************/
/*  go_kilo_models.h                                                      */
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

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace go {

inline constexpr const char *DEFAULT_KILO_GATEWAY_URL = "https://ai-gateway.kilo.ai/api/v1";
inline constexpr const char *DEFAULT_FREE_MODEL = "kilo-auto/free";
inline constexpr const char *ANONYMOUS_API_KEY = "anonymous";
inline constexpr const char *EDITOR_HEADER_NAME = "X-KILOCODE-EDITORNAME";
inline constexpr const char *EDITOR_HEADER_VALUE = "GO. Game Engine (Code Agentics)";
inline constexpr const char *USER_AGENT_BASE = "opencode-kilo-provider";
inline constexpr uint32_t MODELS_FETCH_TIMEOUT_MS = 10000;

struct JsonValue {
	enum class Type {
		NUL,
		BOOL,
		NUMBER,
		STRING,
		ARRAY,
		OBJECT,
	};

	Type type = Type::NUL;
	bool bool_value = false;
	double number_value = 0.0;
	std::string string_value;
	std::vector<JsonValue> array_value;
	std::map<std::string, JsonValue> object_value;

	const JsonValue *get(const std::string &p_key) const;
	std::string as_string(const std::string &p_default = "") const;
	double as_number(double p_default = 0.0) const;
	bool as_bool(bool p_default = false) const;
};

JsonValue parse_json(const std::string &p_text, std::string &r_error);
std::string escape_json_string(const std::string &p_text);

struct GoKiloModel {
	std::string id;
	std::string name;
	bool is_free = false;
	bool supports_tools = false;
	bool has_pricing = false;
	double prompt_price = 0.0;
	double completion_price = 0.0;
	uint64_t context_length = 0;
};

struct KiloHttpError {
	int status = 0;
	int retry_after_seconds = 0;
	std::string message;
	bool is_rate_limit = false;
};

std::vector<GoKiloModel> parse_models_response(const std::string &p_json, std::string &r_error);
bool is_free_model(const GoKiloModel &p_model);
std::vector<GoKiloModel> free_models_for_picker(const std::vector<GoKiloModel> &p_models);
KiloHttpError parse_http_error(int p_status, const std::vector<std::string> &p_headers, const std::string &p_body);
std::string find_header_value(const std::vector<std::string> &p_headers, const std::string &p_name);
std::string models_endpoint(const std::string &p_base_url = DEFAULT_KILO_GATEWAY_URL);
std::string chat_endpoint(const std::string &p_base_url = DEFAULT_KILO_GATEWAY_URL);

} // namespace go
