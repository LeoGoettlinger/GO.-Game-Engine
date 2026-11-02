/**************************************************************************/
/*  go_chat_history.h                                                     */
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

#include <string>
#include <vector>

namespace go {

// Display-only local archive. Never reconstructs executable tool calls or requests.
class ChatHistory {
public:
	struct Entry {
		std::string role;
		std::string text;
	};
	static constexpr size_t MAX_BYTES = 512 * 1024;
	static constexpr size_t MAX_SESSIONS = 20;
	static constexpr size_t MAX_MESSAGES = 200;
	static constexpr size_t MAX_MESSAGE_BYTES = 16000;

private:
	std::vector<std::vector<Entry>> sessions;

public:
	void new_session();
	void append(const std::string &p_role, const std::string &p_text);
	void clear() { sessions.clear(); }
	std::string serialize() const;
	bool load(const std::string &p_json, std::string &r_error);
	const std::vector<std::vector<Entry>> &get_sessions() const { return sessions; }
};

} // namespace go
