/**************************************************************************/
/*  go_commit_message.cpp                                                 */
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

#include "go_commit_message.h"

#include "go_suite_core.h"

#include <algorithm>
#include <sstream>

namespace go {

namespace {

bool is_conventional_prefix(const std::string &p_subject) {
	static const char *prefixes[] = { "feat:", "fix:", "chore:", "docs:", "refactor:", "test:", "perf:", "style:", "build:", "ci:", "revert:" };
	for (const char *prefix : prefixes) {
		if (starts_with(lower(p_subject), prefix)) {
			return true;
		}
	}
	return false;
}

std::string clean_line(std::string p_line) {
	p_line = trim(p_line);
	if (starts_with(p_line, "```")) {
		return "";
	}
	// Remove common markdown/quoting noise from model output.
	while (!p_line.empty() && (p_line.front() == '#' || p_line.front() == '>' || p_line.front() == '*' || p_line.front() == '-' || p_line.front() == '`' || p_line.front() == '"' || p_line.front() == '\'')) {
		p_line.erase(p_line.begin());
		p_line = trim(p_line);
	}
	if (contains(p_line, "```")) {
		p_line.clear();
	}
	return trim(p_line);
}

bool looks_like_secret_value(const std::string &p_line) {
	std::string normalized;
	for (char c : lower(p_line)) {
		if (c != ' ' && c != '\t' && c != '\"' && c != '\'' && c != '_' && c != '-') {
			normalized += c;
		}
	}
	for (const char *marker : { "apikey=", "apikey:", "token=", "token:", "password=", "password:", "secret=", "secret:", "authorization", "bearer", "privatekey", "beginrsa", "ghp", "githubpat" }) {
		if (contains(normalized, marker)) {
			return true;
		}
	}
	return false;
}

bool is_lock_file(const std::string &p_path) {
	const std::string path = lower(p_path);
	return ends_with(path, ".lock") || ends_with(path, "package-lock.json") || ends_with(path, "npm-shrinkwrap.json") || ends_with(path, "bun.lockb");
}

// Fail closed on quoted/escaped/ambiguous Git path syntax instead of guessing.
bool simple_diff_paths(const std::string &p_header, std::string &r_old, std::string &r_new) {
	if (!starts_with(p_header, "diff --git a/") || p_header.find_first_of("\"\\\r\t") != std::string::npos) {
		return false;
	}
	const auto parts = split(p_header.substr(11), ' ');
	if (parts.size() != 2 || !starts_with(parts[0], "a/") || !starts_with(parts[1], "b/")) {
		return false;
	}
	r_old = parts[0].substr(2);
	r_new = parts[1].substr(2);
	return !r_old.empty() && !r_new.empty();
}

std::string bounded_metadata(const std::string &p_text, size_t p_limit) {
	std::string result;
	for (const std::string &line : split(p_text.substr(0, p_limit), '\n')) {
		const std::string safe = looks_like_secret_value(line) ? "[GO. redacted metadata]" : line;
		if (result.size() + safe.size() + 1 > p_limit) {
			break;
		}
		result += safe + "\n";
	}
	return result;
}

} // namespace

std::string fallback_commit_message() {
	return "chore: update project files";
}

bool looks_like_conventional_commit(const std::string &p_subject) {
	return is_conventional_prefix(trim(p_subject));
}

bool looks_like_secret_file(const std::string &p_path) {
	const std::string value = lower(p_path);
	for (const char *marker : { ".env", "secret", "token", "credential", "key", "id_rsa", "id_ed25519", ".pem", ".key", ".p12", ".pfx", "password" }) {
		if (contains(value, marker)) {
			return true;
		}
	}
	return false;
}

std::string sanitize_commit_message(const std::string &p_raw) {
	std::vector<std::string> lines;
	for (const std::string &raw_line : split(p_raw, '\n')) {
		const std::string trimmed = trim(raw_line);
		// Models often wrap the whole message in a code fence. Keep the text, drop the fences.
		if (starts_with(trimmed, "```")) {
			continue;
		}
		std::string line = clean_line(raw_line);
		if (line.empty() || line == "---" || looks_like_secret_value(line)) {
			continue;
		}
		if (starts_with(line, "Subject:") || starts_with(line, "Commit message:")) {
			const size_t colon = line.find(':');
			line = trim(line.substr(colon + 1));
		}
		if (!line.empty()) {
			lines.push_back(line);
		}
	}
	if (lines.empty()) {
		return fallback_commit_message();
	}
	std::string subject = lines.front();
	// Strip a leading conventional-commit marker from markdown examples before re-adding it.
	if (!is_conventional_prefix(subject)) {
		subject = "chore: " + subject;
	}
	if (subject.size() > 72) {
		subject = subject.substr(0, 69) + "...";
	}
	std::ostringstream result;
	result << subject;
	int body_lines = 0;
	for (size_t i = 1; i < lines.size() && body_lines < 10; ++i) {
		std::string body = lines[i];
		if (body.size() > 100) {
			body = body.substr(0, 97) + "...";
		}
		result << "\n\n- " << body;
		body_lines++;
	}
	return result.str();
}

std::string truncate_diff(const std::string &p_diff, size_t p_max_chars, const std::vector<std::string> &p_changed_files) {
	(void)p_changed_files; // Every block is classified using BOTH original Git paths.
	std::string result;
	const auto lines = split(p_diff, '\n');
	auto append = [&](const std::string &p_line) {
		if (p_line.size() + 1 > p_max_chars - result.size()) {
			const std::string marker = "[GO. diff truncated]\n";
			if (marker.size() <= p_max_chars - result.size()) {
				result += marker;
			}
			return false;
		}
		result += p_line + "\n";
		return true;
	};
	for (size_t i = 0; i < lines.size();) {
		// Context lines beginning with a space are NOT new diff headers.
		if (!starts_with(lines[i], "diff --git ")) {
			++i;
			continue;
		}
		size_t end = i + 1;
		while (end < lines.size() && !starts_with(lines[end], "diff --git ")) {
			++end;
		}
		std::string old_path;
		std::string new_path;
		const bool known = simple_diff_paths(lines[i], old_path, new_path);
		bool binary = false;
		bool sensitive_content = false;
		for (size_t j = i + 1; j < end; ++j) {
			binary = binary || starts_with(lines[j], "Binary files ") || lines[j] == "GIT binary patch";
			sensitive_content = sensitive_content || looks_like_secret_value(lines[j]);
		}
		if (!known) {
			if (!append("[GO. omitted diff with ambiguous or quoted paths]")) {
				break;
			}
		} else if (looks_like_secret_file(old_path) || looks_like_secret_file(new_path)) {
			if (!append("[GO. redacted sensitive file: " + (looks_like_secret_file(old_path) ? old_path : new_path) + "]")) {
				break;
			}
		} else if (sensitive_content) {
			if (!append("[GO. redacted content in file: " + new_path + "]")) {
				break;
			}
		} else if (binary || is_lock_file(old_path) || is_lock_file(new_path)) {
			if (!append("[GO. binary/lock file: " + new_path + "]")) {
				break;
			}
		} else {
			for (size_t j = i; j < end; ++j) {
				if (!append(looks_like_secret_value(lines[j]) ? "[GO. redacted line]" : lines[j])) {
					return result;
				}
			}
		}
		i = end;
	}
	return result;
}

std::string build_commit_prompt(const std::string &p_status, const std::string &p_diff_stat, const std::string &p_diff) {
	std::ostringstream prompt;
	prompt << "You are generating one git commit message for a Godot project.\n"
		   << "Reply with only the commit message. Use Conventional Commits.\n"
		   << "Subject <= 72 characters, imperative mood, then at most 10 short bullet lines.\n"
		   << "Do not invent features that are not visible in the diff.\n"
		   << "Do not include secrets, credentials, code fences or markdown headings.\n\n"
		   << "STATUS:\n"
		   << bounded_metadata(p_status, 2000) << "\n\nDIFF STAT:\n"
		   << bounded_metadata(p_diff_stat, 3000)
		   << "\n\nDIFF (redacted/truncated):\n"
		   << truncate_diff(p_diff, 12000) << "\n";
	return prompt.str();
}

} // namespace go
