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
	const std::string line = lower(p_line);
	for (const char *marker : { "api_key=", "apikey=", "token=", "password=", "secret=", "bearer ", "private key", "begin rsa" }) {
		if (contains(line, marker)) {
			return true;
		}
	}
	return false;
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
	for (const char *marker : { ".env", "secret", "token", "credential", "id_rsa", "id_ed25519", ".pem", ".key", ".p12", ".pfx", "password" }) {
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
	std::ostringstream result;
	std::string current_file;
	bool redacted_current = false;
	size_t used = 0;
	size_t line_count = 0;
	for (const std::string &line : split(p_diff, '\n')) {
		const std::string header = trim(line);
		if (starts_with(header, "diff --git ")) {
			const size_t b_marker = header.find(" b/");
			current_file = b_marker == std::string::npos ? "" : header.substr(b_marker + 3);
			redacted_current = false;
		} else if (starts_with(header, "+++ b/")) {
			current_file = header.substr(6);
		}
		if (!current_file.empty() && looks_like_secret_file(current_file)) {
			if (!redacted_current) {
				result << "[GO. redacted sensitive file: " << current_file << "]\n";
				redacted_current = true;
			}
			continue;
		}
		bool redact = false;
		for (const std::string &path : p_changed_files) {
			if (looks_like_secret_file(path) && (path == current_file || contains(line, path))) {
				redact = true;
				break;
			}
		}
		if (redact || looks_like_secret_value(line)) {
			result << "[GO. redacted line]\n";
			continue;
		}
		if (used + line.size() + 1 > p_max_chars) {
			result << "\n[GO. diff truncated after " << p_max_chars << " characters]\n";
			return result.str();
		}
		result << line << '\n';
		used += line.size() + 1;
		line_count++;
		if (line_count > 4000) {
			result << "\n[GO. diff truncated after 4000 lines]\n";
			break;
		}
	}
	return result.str();
}

std::string build_commit_prompt(const std::string &p_status, const std::string &p_diff_stat, const std::string &p_diff) {
	std::ostringstream prompt;
	prompt << "You are generating one git commit message for a Godot project.\n"
		   << "Reply with only the commit message. Use Conventional Commits.\n"
		   << "Subject <= 72 characters, imperative mood, then at most 10 short bullet lines.\n"
		   << "Do not invent features that are not visible in the diff.\n"
		   << "Do not include secrets, credentials, code fences or markdown headings.\n\n"
		   << "STATUS:\n" << p_status << "\n\nDIFF STAT:\n" << p_diff_stat
		   << "\n\nDIFF (redacted/truncated):\n" << truncate_diff(p_diff, 12000) << "\n";
	return prompt.str();
}

} // namespace go
