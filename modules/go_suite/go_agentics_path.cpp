// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_agentics_path.h"

#include "go_suite_core.h"

#include <cctype>

namespace go {

namespace {

bool has_control_character(const std::string &p_text) {
	for (char c : p_text) {
		if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) == 127) {
			return true;
		}
	}
	return false;
}

bool normalize(const std::string &p_path, std::string &r_relative, std::string &r_error) {
	const std::string path = trim(p_path);
	if (path.empty()) {
		r_error = "path is empty";
		return false;
	}
	if (has_control_character(path) || contains(path, "\\")) {
		r_error = "only project-relative res:// paths with forward slashes are allowed";
		return false;
	}
	if (!starts_with(lower(path), "res://")) {
		r_error = "absolute paths, user:// paths and foreign URL schemes are not allowed";
		return false;
	}
	std::string rest = path.substr(6);
	if (contains(rest, ":")) {
		r_error = "drive letters and URL schemes inside res:// are not allowed";
		return false;
	}
	if (starts_with(rest, "/")) {
		r_error = "res:/// absolute paths are not allowed";
		return false;
	}
	if (contains(rest, "://")) {
		r_error = "nested URL schemes are not allowed";
		return false;
	}
	std::vector<std::string> parts;
	for (const std::string &segment : split(rest, '/')) {
		if (segment.empty() || segment == ".") {
			continue;
		}
		if (segment == "..") {
			r_error = "path traversal with '..' is not allowed";
			return false;
		}
		if (has_control_character(segment)) {
			r_error = "path contains control characters";
			return false;
		}
		parts.push_back(segment);
	}
	r_relative = join(parts, "/");
	return true;
}

bool is_blocked_git_or_cache(const std::string &p_relative) {
	return p_relative == ".git" || starts_with(p_relative, ".git/") ||
			p_relative == ".godot" || starts_with(p_relative, ".godot/") ||
			contains(p_relative, "/.git/") || ends_with(p_relative, "/.git") ||
			contains(p_relative, "/.godot/") || ends_with(p_relative, "/.godot");
}

} // namespace

bool is_write_operation(PathOperation p_operation) {
	return p_operation == PathOperation::WRITE || p_operation == PathOperation::DELETE;
}

bool validate_symlink_target(const std::string &p_target, std::string &r_error) {
	const std::string target = trim(p_target);
	if (target.empty()) {
		r_error = "symlink target is empty";
		return false;
	}
	if (starts_with(lower(target), "res://")) {
		std::string relative;
		if (!normalize(target, relative, r_error)) {
			return false;
		}
		if (is_blocked_git_or_cache(relative)) {
			r_error = "symlink target points into Git or editor cache data";
			return false;
		}
		return true;
	}
	if (starts_with(target, "/") || starts_with(target, "\\") || contains(target, "://") ||
			contains(target, "..") || contains(target, ":")) {
		r_error = "symlink target escapes the project";
		return false;
	}
	return true;
}

PathValidationResult validate_res_path(const std::string &p_path, PathOperation p_operation, const SymlinkMap &p_symlinks) {
	PathValidationResult result;
	std::string relative;
	if (!normalize(p_path, relative, result.error)) {
		return result;
	}
	if (is_blocked_git_or_cache(relative)) {
		result.error = "Git internals and .godot cache are not source files";
		return result;
	}
	if (is_write_operation(p_operation) && relative.empty()) {
		result.error = "the project root cannot be written or deleted";
		return result;
	}
	for (const auto &entry : p_symlinks) {
		const std::string link = entry.first;
		if (relative == link || starts_with(relative, link + "/")) {
			if (!validate_symlink_target(entry.second, result.error)) {
				return result;
			}
		}
	}
	result.ok = true;
	result.relative = relative;
	result.normalized = "res://" + relative;
	return result;
}

bool is_safe_res_path(const std::string &p_path, PathOperation p_operation, const SymlinkMap &p_symlinks) {
	return validate_res_path(p_path, p_operation, p_symlinks).ok;
}

} // namespace go
