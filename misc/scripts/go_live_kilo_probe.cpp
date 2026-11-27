/**************************************************************************/
/*  go_live_kilo_probe.cpp                                                */
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

// Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
// SPDX-License-Identifier: MIT
#include "go_agentics_tools.h"
#include "go_kilo_models.h"

#include <iostream>
#include <iterator>

int main(int argc, char **argv) {
	if (argc != 2) {
		return 2;
	}
	const std::string mode = argv[1];
	const std::string input((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
	if (input.size() > 4 * 1024 * 1024) {
		return 2;
	}
	std::string error;
	if (mode == "prepare") {
		const auto models = go::parse_models_response(input, error);
		if (!error.empty()) {
			std::cerr << "Invalid live model catalog";
			return 1;
		}
		for (const auto &model : models) {
			if (model.id != go::DEFAULT_FREE_MODEL || !go::is_free_model(model)) {
				continue;
			}
			const std::string definitions = R"([{"type":"function","function":{"name":"list_files","description":"List files in the synthetic empty project","parameters":{"type":"object","properties":{"path":{"type":"string"}},"required":["path"],"additionalProperties":false}}}])";
			const std::vector<go::GoKiloMessage> messages = {
				{ "system", "This is a synthetic GO. Game Engine integration test. There are no user files or credentials. Request exactly one list_files action with path res://. Do not invent results. Use a native tool call when available; otherwise return exactly {\"tool\":\"list_files\",\"arguments\":{\"path\":\"res://\"}}.", {}, "" },
				{ "user", "Inspect the synthetic empty project using list_files now.", {}, "" },
			};
			std::cout << go::build_conversation_request_json(model.id, messages, model.supports_tools, definitions, error);
			return error.empty() ? 0 : 1;
		}
		std::cerr << "Anonymous default model absent or not verified free; refusing chat request";
		return 1;
	}
	if (mode == "check") {
		const auto response = go::parse_chat_response(input, error);
		const auto calls = go::parse_tool_calls_json(response.tool_calls_json);
		if (!error.empty() || !response.error.empty() || calls.size() != 1 || calls[0].name != "list_files" || !go::GoAgenticsTools::validate_arguments(calls[0].name, calls[0].arguments).empty()) {
			std::cerr << "Live response did not contain the required single valid tool plan";
			return 1;
		}
		const auto arguments = go::parse_json(calls[0].arguments, error);
		const auto *path = arguments.get("path");
		if (!error.empty() || path == nullptr || path->as_string() != "res://") {
			return 1;
		}
		std::cout << "GO_LIVE_KILO_PLAN_OK: " << (response.used_json_fallback ? "JSON fallback" : "native tool") << "; production parser/schema checks; no editor execution\n";
		return 0;
	}
	return 2;
}
