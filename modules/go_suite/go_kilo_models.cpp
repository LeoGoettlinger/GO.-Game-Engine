/**************************************************************************/
/*  go_kilo_models.cpp                                                    */
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

#include "go_kilo_models.h"

#include "go_suite_core.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <set>
#include <sstream>

namespace go {

namespace {

void append_utf8(uint32_t p_codepoint, std::string &r_text) {
	if (p_codepoint <= 0x7F) {
		r_text.push_back(static_cast<char>(p_codepoint));
	} else if (p_codepoint <= 0x7FF) {
		r_text.push_back(static_cast<char>(0xC0 | (p_codepoint >> 6)));
		r_text.push_back(static_cast<char>(0x80 | (p_codepoint & 0x3F)));
	} else if (p_codepoint <= 0xFFFF) {
		r_text.push_back(static_cast<char>(0xE0 | (p_codepoint >> 12)));
		r_text.push_back(static_cast<char>(0x80 | ((p_codepoint >> 6) & 0x3F)));
		r_text.push_back(static_cast<char>(0x80 | (p_codepoint & 0x3F)));
	} else {
		r_text.push_back(static_cast<char>(0xF0 | (p_codepoint >> 18)));
		r_text.push_back(static_cast<char>(0x80 | ((p_codepoint >> 12) & 0x3F)));
		r_text.push_back(static_cast<char>(0x80 | ((p_codepoint >> 6) & 0x3F)));
		r_text.push_back(static_cast<char>(0x80 | (p_codepoint & 0x3F)));
	}
}

int hex_value(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

class JsonParser {
	const std::string &text;
	size_t pos = 0;
	unsigned depth = 0;
	std::string error;

public:
	explicit JsonParser(const std::string &p_text) :
			text(p_text) {}

	const std::string &get_error() const { return error; }

	bool parse(JsonValue &r_value) {
		skip_ws();
		if (!parse_value(r_value)) {
			return false;
		}
		skip_ws();
		if (pos != text.size()) {
			error = "trailing characters after JSON value";
			return false;
		}
		return true;
	}

private:
	void fail(const std::string &p_message) {
		if (error.empty()) {
			std::ostringstream stream;
			stream << p_message << " at byte " << pos;
			error = stream.str();
		}
	}

	void skip_ws() {
		while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\r' || text[pos] == '\n')) {
			pos++;
		}
	}

	bool consume(char p_expected) {
		if (pos < text.size() && text[pos] == p_expected) {
			pos++;
			return true;
		}
		fail(std::string("expected '") + p_expected + "'");
		return false;
	}

	bool parse_literal(const std::string &p_literal, JsonValue &r_value) {
		if (text.compare(pos, p_literal.size(), p_literal) != 0) {
			fail("invalid literal");
			return false;
		}
		pos += p_literal.size();
		if (p_literal == "true") {
			r_value.type = JsonValue::Type::BOOL;
			r_value.bool_value = true;
		} else if (p_literal == "false") {
			r_value.type = JsonValue::Type::BOOL;
			r_value.bool_value = false;
		} else {
			r_value.type = JsonValue::Type::NUL;
		}
		return true;
	}

	bool parse_string(std::string &r_value) {
		if (!consume('"')) {
			return false;
		}
		std::string result;
		while (pos < text.size()) {
			const char c = text[pos++];
			if (c == '"') {
				r_value = result;
				return true;
			}
			if (c == '\\') {
				if (pos >= text.size()) {
					fail("unterminated escape");
					return false;
				}
				const char escaped = text[pos++];
				switch (escaped) {
					case '"':
						result.push_back('"');
						break;
					case '\\':
						result.push_back('\\');
						break;
					case '/':
						result.push_back('/');
						break;
					case 'b':
						result.push_back('\b');
						break;
					case 'f':
						result.push_back('\f');
						break;
					case 'n':
						result.push_back('\n');
						break;
					case 'r':
						result.push_back('\r');
						break;
					case 't':
						result.push_back('\t');
						break;
					case 'u': {
						if (pos + 4 > text.size()) {
							fail("unterminated unicode escape");
							return false;
						}
						uint32_t codepoint = 0;
						for (int i = 0; i < 4; ++i) {
							const int value = hex_value(text[pos++]);
							if (value < 0) {
								fail("invalid unicode escape");
								return false;
							}
							codepoint = codepoint * 16 + static_cast<uint32_t>(value);
						}
						if (codepoint >= 0xD800 && codepoint <= 0xDBFF && pos + 6 <= text.size() && text[pos] == '\\' && text[pos + 1] == 'u') {
							pos += 2;
							uint32_t low = 0;
							bool valid = true;
							for (int i = 0; i < 4; ++i) {
								const int value = hex_value(text[pos++]);
								if (value < 0) {
									valid = false;
									break;
								}
								low = low * 16 + static_cast<uint32_t>(value);
							}
							if (valid && low >= 0xDC00 && low <= 0xDFFF) {
								codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
							} else {
								codepoint = 0xFFFD;
							}
						}
						append_utf8(codepoint, result);
					} break;
					default:
						fail("invalid escape character");
						return false;
				}
				continue;
			}
			if (static_cast<unsigned char>(c) < 32) {
				fail("control character in string");
				return false;
			}
			result.push_back(c);
		}
		fail("unterminated string");
		return false;
	}

	bool parse_number(JsonValue &r_value) {
		const size_t start = pos;
		while (pos < text.size()) {
			const char c = text[pos];
			if (std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
				pos++;
			} else {
				break;
			}
		}
		if (start == pos) {
			fail("invalid number");
			return false;
		}
		const std::string token = text.substr(start, pos - start);
		char *end = nullptr;
		const double value = std::strtod(token.c_str(), &end);
		if (end != token.c_str() + token.size()) {
			fail("malformed number");
			return false;
		}
		r_value.type = JsonValue::Type::NUMBER;
		r_value.number_value = value;
		return true;
	}

	bool parse_array(JsonValue &r_value) {
		r_value.type = JsonValue::Type::ARRAY;
		if (!consume('[')) {
			return false;
		}
		skip_ws();
		if (pos < text.size() && text[pos] == ']') {
			pos++;
			return true;
		}
		while (true) {
			JsonValue item;
			if (!parse_value(item)) {
				return false;
			}
			r_value.array_value.push_back(item);
			skip_ws();
			if (pos < text.size() && text[pos] == ',') {
				pos++;
				skip_ws();
				continue;
			}
			return consume(']');
		}
	}

	bool parse_object(JsonValue &r_value) {
		r_value.type = JsonValue::Type::OBJECT;
		if (!consume('{')) {
			return false;
		}
		skip_ws();
		if (pos < text.size() && text[pos] == '}') {
			pos++;
			return true;
		}
		while (true) {
			std::string key;
			if (!parse_string(key)) {
				return false;
			}
			skip_ws();
			if (!consume(':')) {
				return false;
			}
			JsonValue value;
			if (!parse_value(value)) {
				return false;
			}
			r_value.object_value[key] = value;
			skip_ws();
			if (pos < text.size() && text[pos] == ',') {
				pos++;
				skip_ws();
				continue;
			}
			return consume('}');
		}
	}

	bool parse_value(JsonValue &r_value) {
		if (depth >= 64) {
			fail("JSON nesting limit exceeded");
			return false;
		}
		struct DepthGuard {
			unsigned &value;
			explicit DepthGuard(unsigned &p_value) : value(p_value) { ++value; }
			~DepthGuard() { --value; }
		} guard(depth);
		skip_ws();
		if (pos >= text.size()) {
			fail("unexpected end of JSON");
			return false;
		}
		const char c = text[pos];
		switch (c) {
			case '{':
				return parse_object(r_value);
			case '[':
				return parse_array(r_value);
			case '"': {
				r_value.type = JsonValue::Type::STRING;
				return parse_string(r_value.string_value);
			}
			case 't':
				return parse_literal("true", r_value);
			case 'f':
				return parse_literal("false", r_value);
			case 'n':
				return parse_literal("null", r_value);
			default:
				return parse_number(r_value);
		}
	}
};

double price_from_json(const JsonValue *p_value) {
	if (p_value == nullptr) {
		return 1.0;
	}
	if (p_value->type == JsonValue::Type::NUMBER) {
		return p_value->number_value;
	}
	if (p_value->type == JsonValue::Type::STRING) {
		const std::string value = trim(p_value->string_value);
		char *end = nullptr;
		const double result = std::strtod(value.c_str(), &end);
		if (value.empty() || end == value.c_str() || end != value.c_str() + value.size() || !std::isfinite(result) || result < 0.0) {
			return 1.0;
		}
		return result;
	}
	return 1.0;
}

std::string json_value_to_string(const JsonValue &p_value) {
	std::ostringstream stream;
	switch (p_value.type) {
		case JsonValue::Type::NUL:
			return "null";
		case JsonValue::Type::BOOL:
			return p_value.bool_value ? "true" : "false";
		case JsonValue::Type::NUMBER:
			stream << std::setprecision(17) << p_value.number_value;
			return stream.str();
		case JsonValue::Type::STRING:
			return escape_json_string(p_value.string_value);
		case JsonValue::Type::ARRAY:
			stream << "[";
			for (size_t i = 0; i < p_value.array_value.size(); ++i) {
				if (i > 0) {
					stream << ",";
				}
				stream << json_value_to_string(p_value.array_value[i]);
			}
			stream << "]";
			return stream.str();
		case JsonValue::Type::OBJECT:
			stream << "{";
			bool first = true;
			for (const auto &entry : p_value.object_value) {
				if (!first) {
					stream << ",";
				}
				first = false;
				stream << escape_json_string(entry.first) << ":" << json_value_to_string(entry.second);
			}
			stream << "}";
			return stream.str();
	}
	return "null";
}

std::string extract_first_json_object(const std::string &p_text) {
	std::string text = trim(p_text);
	const std::string fence = "```";
	const size_t fence_start = text.find(fence);
	if (fence_start != std::string::npos) {
		const size_t line_end = text.find('\n', fence_start);
		if (line_end != std::string::npos) {
			const size_t fence_end = text.find(fence, line_end);
			if (fence_end != std::string::npos) {
				text = trim(text.substr(line_end + 1, fence_end - line_end - 1));
			}
		}
	}
	const size_t start = text.find('{');
	const size_t end = text.rfind('}');
	if (start == std::string::npos || end == std::string::npos || end <= start) {
		return "";
	}
	return text.substr(start, end - start + 1);
}

std::string serialise_tool_calls_impl(const std::vector<GoKiloToolCall> &p_tool_calls) {
	std::ostringstream stream;
	stream << "[";
	for (size_t i = 0; i < p_tool_calls.size(); ++i) {
		if (i > 0) {
			stream << ",";
		}
		stream << "{\"id\":" << escape_json_string(p_tool_calls[i].id)
			   << ",\"name\":" << escape_json_string(p_tool_calls[i].name)
			   << ",\"arguments\":" << escape_json_string(p_tool_calls[i].arguments) << "}";
	}
	stream << "]";
	return stream.str();
}

} // namespace

const JsonValue *JsonValue::get(const std::string &p_key) const {
	if (type != Type::OBJECT) {
		return nullptr;
	}
	const auto found = object_value.find(p_key);
	return found == object_value.end() ? nullptr : &found->second;
}

std::string JsonValue::as_string(const std::string &p_default) const {
	return type == Type::STRING ? string_value : p_default;
}

double JsonValue::as_number(double p_default) const {
	return type == Type::NUMBER ? number_value : p_default;
}

bool JsonValue::as_bool(bool p_default) const {
	return type == Type::BOOL ? bool_value : p_default;
}

JsonValue parse_json(const std::string &p_text, std::string &r_error) {
	JsonValue value;
	JsonParser parser(p_text);
	if (!parser.parse(value)) {
		r_error = parser.get_error();
		return JsonValue();
	}
	r_error.clear();
	return value;
}

std::string escape_json_string(const std::string &p_text) {
	std::ostringstream stream;
	stream << '"';
	for (const char c : p_text) {
		switch (c) {
			case '"':
				stream << "\\\"";
				break;
			case '\\':
				stream << "\\\\";
				break;
			case '\b':
				stream << "\\b";
				break;
			case '\f':
				stream << "\\f";
				break;
			case '\n':
				stream << "\\n";
				break;
			case '\r':
				stream << "\\r";
				break;
			case '\t':
				stream << "\\t";
				break;
			default:
				if (static_cast<unsigned char>(c) < 32) {
					stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c) << std::dec;
				} else {
					stream << c;
				}
		}
	}
	stream << '"';
	return stream.str();
}

std::vector<GoKiloModel> parse_models_response(const std::string &p_json, std::string &r_error) {
	std::vector<GoKiloModel> models;
	const JsonValue root = parse_json(p_json, r_error);
	if (!r_error.empty()) {
		return models;
	}
	const JsonValue *data = root.get("data");
	if (data == nullptr || data->type != JsonValue::Type::ARRAY) {
		r_error = "gateway /models response is missing the data array";
		return models;
	}
	for (const JsonValue &item : data->array_value) {
		if (item.type != JsonValue::Type::OBJECT) {
			continue;
		}
		GoKiloModel model;
		const JsonValue *id = item.get("id");
		if (id == nullptr) {
			continue;
		}
		model.id = id->as_string();
		if (model.id.empty()) {
			continue;
		}
		const JsonValue *name = item.get("name");
		model.name = name != nullptr ? name->as_string(model.id) : model.id;
		const JsonValue *free = item.get("isFree");
		if (free == nullptr) {
			free = item.get("is_free");
		}
		model.is_free = free != nullptr && free->as_bool(false);
		const JsonValue *context = item.get("context_length");
		if (context != nullptr && context->type == JsonValue::Type::NUMBER) {
			model.context_length = static_cast<uint64_t>(context->number_value);
		}
		const JsonValue *supported = item.get("supported_parameters");
		if (supported != nullptr && supported->type == JsonValue::Type::ARRAY) {
			for (const JsonValue &parameter : supported->array_value) {
				if (parameter.as_string() == "tools") {
					model.supports_tools = true;
					break;
				}
			}
		}
		const JsonValue *pricing = item.get("pricing");
		if (pricing != nullptr && pricing->type == JsonValue::Type::OBJECT) {
			const JsonValue *prompt = pricing->get("prompt");
			const JsonValue *completion = pricing->get("completion");
			if (prompt != nullptr && completion != nullptr) {
				model.has_pricing = true;
				model.prompt_price = price_from_json(prompt);
				model.completion_price = price_from_json(completion);
			}
		} else if (pricing != nullptr && pricing->as_string() == "free") {
			model.has_pricing = true;
		}
		models.push_back(model);
	}
	return models;
}

bool is_free_model(const GoKiloModel &p_model) {
	if (p_model.id == DEFAULT_FREE_MODEL || ends_with(p_model.id, ":free")) {
		return true;
	}
	if (p_model.is_free) {
		return true;
	}
	return p_model.has_pricing && p_model.prompt_price == 0.0 && p_model.completion_price == 0.0;
}

bool is_free_model_id(const std::string &p_model_id) {
	if (p_model_id.empty() || contains(p_model_id, " ") || p_model_id.size() > 128) {
		return false;
	}
	return p_model_id == DEFAULT_FREE_MODEL || ends_with(p_model_id, ":free");
}

std::vector<GoKiloModel> free_models_for_picker(const std::vector<GoKiloModel> &p_models) {
	std::vector<GoKiloModel> result;
	GoKiloModel default_model;
	default_model.id = DEFAULT_FREE_MODEL;
	default_model.name = "Kilo Auto (free)";
	default_model.is_free = true;
	for (const GoKiloModel &model : p_models) {
		if (model.id == DEFAULT_FREE_MODEL) {
			default_model = model;
			default_model.is_free = true;
			break;
		}
	}
	result.push_back(default_model);
	for (const GoKiloModel &model : p_models) {
		if (!is_free_model(model) || model.id == DEFAULT_FREE_MODEL) {
			continue;
		}
		bool duplicate = false;
		for (const GoKiloModel &existing : result) {
			if (existing.id == model.id) {
				duplicate = true;
				break;
			}
		}
		if (!duplicate) {
			result.push_back(model);
		}
	}
	return result;
}

std::string find_header_value(const std::vector<std::string> &p_headers, const std::string &p_name) {
	const std::string wanted = lower(p_name);
	for (const std::string &header : p_headers) {
		const size_t colon = header.find(':');
		if (colon == std::string::npos) {
			continue;
		}
		if (lower(trim(header.substr(0, colon))) == wanted) {
			return trim(header.substr(colon + 1));
		}
	}
	return "";
}

KiloHttpError parse_http_error(int p_status, const std::vector<std::string> &p_headers, const std::string &p_body) {
	KiloHttpError result;
	result.status = p_status;
	result.is_rate_limit = p_status == 429;
	const std::string retry_after = find_header_value(p_headers, "Retry-After");
	if (!retry_after.empty()) {
		char *end = nullptr;
		const long seconds = std::strtol(retry_after.c_str(), &end, 10);
		if (end != retry_after.c_str() && seconds > 0) {
			result.retry_after_seconds = static_cast<int>(seconds);
		}
	}
	std::string parse_error;
	const JsonValue body = parse_json(p_body, parse_error);
	if (parse_error.empty()) {
		const JsonValue *error_object = body.get("error");
		if (error_object != nullptr && error_object->type == JsonValue::Type::OBJECT) {
			const JsonValue *message = error_object->get("message");
			result.message = message != nullptr ? message->as_string() : "";
		}
		if (result.message.empty()) {
			const JsonValue *message = body.get("message");
			result.message = message != nullptr ? message->as_string() : "";
		}
	}
	if (result.message.empty()) {
		if (result.is_rate_limit) {
			result.message = "Kilo Gateway anonymous rate limit reached (about 200 requests/hour/IP).";
		} else if (p_status >= 500) {
			result.message = "Kilo Gateway is temporarily unavailable.";
		} else {
			result.message = "Kilo Gateway request failed.";
		}
	}
	return result;
}

std::string models_endpoint(const std::string &p_base_url) {
	return strip_suffix(p_base_url, "/") + "/models";
}

std::string chat_endpoint(const std::string &p_base_url) {
	return strip_suffix(p_base_url, "/") + "/chat/completions";
}

std::string sanitize_chat_content(const std::string &p_text, size_t p_max_chars) {
	if (p_text.size() <= p_max_chars) {
		return p_text;
	}
	return p_text.substr(0, p_max_chars) + "\n[truncated by GO.]";
}

std::string build_conversation_request_json(const std::string &p_model,
		const std::vector<GoKiloMessage> &p_messages, bool p_tools_enabled,
		const std::string &p_tool_definitions_json, std::string &r_error) {
	r_error.clear();
	auto fail = [&r_error](const std::string &p_reason) -> std::string {
		r_error = p_reason;
		return "";
	};
	if (p_messages.empty() || p_messages.size() > 256) {
		return fail("Conversation must contain between 1 and 256 messages");
	}
	if (p_tools_enabled) {
		const JsonValue definitions = parse_json(p_tool_definitions_json, r_error);
		if (!r_error.empty() || definitions.type != JsonValue::Type::ARRAY || definitions.array_value.empty()) {
			return fail("Tool definitions must be a nonempty JSON array");
		}
		for (const JsonValue &definition : definitions.array_value) {
			const JsonValue *type = definition.get("type");
			const JsonValue *function = definition.get("function");
			if (type == nullptr || type->as_string() != "function" || function == nullptr || function->get("name") == nullptr || function->get("name")->as_string().empty()) {
				return fail("Invalid native function definition");
			}
		}
	}
	std::set<std::string> pending;
	std::set<std::string> seen;
	size_t rounds = 0;
	std::ostringstream stream;
	stream << "{\"model\":" << escape_json_string(p_model) << ",\"stream\":false,\"messages\":[";
	for (size_t i = 0; i < p_messages.size(); ++i) {
		const GoKiloMessage &message = p_messages[i];
		if (message.role != "system" && message.role != "user" && message.role != "assistant" && message.role != "tool") {
			return fail("Unsupported message role");
		}
		if (message.role == "tool") {
			if (!message.tool_calls.empty() || pending.erase(message.tool_call_id) != 1) {
				return fail("Tool result has no matching pending call");
			}
		} else if (!pending.empty() || !message.tool_call_id.empty()) {
			return fail("All tool results must precede the next conversation message");
		}
		if (!message.tool_calls.empty() && (message.role != "assistant" || ++rounds > 20 || message.tool_calls.size() > 20)) {
			return fail("Invalid assistant tool round or tool limit exceeded");
		}
		if (i > 0) {
			stream << ",";
		}
		stream << "{\"role\":" << escape_json_string(message.role)
			   << ",\"content\":" << escape_json_string(sanitize_chat_content(message.content));
		if (message.role == "tool") {
			stream << ",\"tool_call_id\":" << escape_json_string(message.tool_call_id);
		}
		if (!message.tool_calls.empty()) {
			stream << ",\"tool_calls\":[";
			for (size_t j = 0; j < message.tool_calls.size(); ++j) {
				const GoKiloToolCall &call = message.tool_calls[j];
				std::string error;
				const JsonValue arguments = parse_json(call.arguments, error);
				if (call.id.empty() || call.id.size() > 256 || call.name.empty() || call.name.size() > 128 || !seen.insert(call.id).second || !error.empty() || arguments.type != JsonValue::Type::OBJECT) {
					return fail("Invalid or duplicate tool call identity or arguments");
				}
				pending.insert(call.id);
				if (j > 0) {
					stream << ",";
				}
				stream << "{\"id\":" << escape_json_string(call.id) << ",\"type\":\"function\",\"function\":{\"name\":"
					   << escape_json_string(call.name) << ",\"arguments\":" << escape_json_string(call.arguments) << "}}";
			}
			stream << "]";
		}
		stream << "}";
	}
	if (!pending.empty()) {
		return fail("Conversation has unresolved tool calls");
	}
	stream << "]";
	if (p_tools_enabled) {
		stream << ",\"tools\":" << p_tool_definitions_json << ",\"tool_choice\":\"auto\"";
	}
	stream << "}";
	return stream.str();
}

std::string build_chat_request_json(
		const std::string &p_model,
		const std::vector<std::pair<std::string, std::string>> &p_messages,
		bool p_tools_enabled,
		const std::string &p_tool_definitions_json) {
	std::ostringstream stream;
	stream << "{\"model\":" << escape_json_string(p_model) << ",\"stream\":false,\"messages\":[";
	for (size_t i = 0; i < p_messages.size(); ++i) {
		if (i > 0) {
			stream << ",";
		}
		stream << "{\"role\":" << escape_json_string(p_messages[i].first)
			   << ",\"content\":" << escape_json_string(sanitize_chat_content(p_messages[i].second)) << "}";
	}
	stream << "]";
	if (p_tools_enabled && !trim(p_tool_definitions_json).empty()) {
		stream << ",\"tools\":" << trim(p_tool_definitions_json) << ",\"tool_choice\":\"auto\"";
	}
	stream << "}";
	return stream.str();
}

GoKiloChatResponse parse_chat_response(const std::string &p_json, std::string &r_error) {
	GoKiloChatResponse result;
	r_error.clear();
	const JsonValue root = parse_json(p_json, r_error);
	if (!r_error.empty()) {
		result.error = r_error;
		return result;
	}
	const JsonValue *choices = root.get("choices");
	if (choices == nullptr || choices->type != JsonValue::Type::ARRAY || choices->array_value.empty()) {
		result.error = "gateway chat response has no choices";
		return result;
	}
	const JsonValue *message = choices->array_value[0].get("message");
	if (message == nullptr || message->type != JsonValue::Type::OBJECT) {
		result.error = "gateway chat response has no message";
		return result;
	}
	const JsonValue *content = message->get("content");
	result.content = content != nullptr ? sanitize_chat_content(content->as_string()) : "";

	std::vector<GoKiloToolCall> calls;
	const JsonValue *native_calls = message->get("tool_calls");
	if (native_calls != nullptr && native_calls->type == JsonValue::Type::ARRAY) {
		for (const JsonValue &call : native_calls->array_value) {
			const JsonValue *function = call.get("function");
			const JsonValue *name = function != nullptr ? function->get("name") : nullptr;
			if (name == nullptr || name->as_string().empty()) {
				continue;
			}
			GoKiloToolCall parsed;
			const JsonValue *id = call.get("id");
			parsed.id = id != nullptr ? id->as_string() : "native-" + std::to_string(calls.size());
			parsed.name = name->as_string();
			const JsonValue *arguments = function->get("arguments");
			parsed.arguments = arguments == nullptr ? "{}" : (arguments->type == JsonValue::Type::STRING ? arguments->as_string() : json_value_to_string(*arguments));
			calls.push_back(parsed);
		}
	}

	// Models without native tool calling may return a compact JSON object instead.
	if (calls.empty()) {
		const std::string candidate = extract_first_json_object(result.content);
		if (!candidate.empty()) {
			std::string fallback_error;
			const JsonValue fallback = parse_json(candidate, fallback_error);
			if (fallback_error.empty()) {
				const JsonValue *tool = fallback.get("tool");
				const JsonValue *arguments = fallback.get("arguments");
				if (tool != nullptr && !tool->as_string().empty()) {
					GoKiloToolCall parsed;
					parsed.id = "json-fallback-0";
					parsed.name = tool->as_string();
					parsed.arguments = arguments == nullptr ? "{}" : (arguments->type == JsonValue::Type::STRING ? arguments->as_string() : json_value_to_string(*arguments));
					calls.push_back(parsed);
					result.used_json_fallback = true;
				}
			}
		}
	}

	result.tool_calls_json = serialise_tool_calls_impl(calls);
	return result;
}

std::vector<GoKiloToolCall> parse_tool_calls_json(const std::string &p_json) {
	std::vector<GoKiloToolCall> result;
	std::string error;
	const JsonValue root = parse_json(p_json, error);
	if (!error.empty() || root.type != JsonValue::Type::ARRAY) {
		return result;
	}
	for (const JsonValue &item : root.array_value) {
		GoKiloToolCall call;
		const JsonValue *id = item.get("id");
		const JsonValue *name = item.get("name");
		const JsonValue *arguments = item.get("arguments");
		call.id = id != nullptr ? id->as_string() : "";
		call.name = name != nullptr ? name->as_string() : "";
		call.arguments = arguments != nullptr ? arguments->as_string("{}") : "{}";
		if (!call.name.empty()) {
			result.push_back(call);
		}
	}
	return result;
}

std::string serialise_tool_calls(const std::vector<GoKiloToolCall> &p_tool_calls) {
	return serialise_tool_calls_impl(p_tool_calls);
}

} // namespace go
