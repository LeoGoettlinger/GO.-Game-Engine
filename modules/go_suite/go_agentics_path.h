// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace go {

enum class PathOperation {
	READ,
	WRITE,
	DELETE,
};

struct PathValidationResult {
	bool ok = false;
	std::string normalized; // res://... form
	std::string relative; // project-relative path
	std::string error;
};

using SymlinkMap = std::vector<std::pair<std::string, std::string>>;

PathValidationResult validate_res_path(const std::string &p_path, PathOperation p_operation, const SymlinkMap &p_symlinks = {});
bool is_write_operation(PathOperation p_operation);
bool is_safe_res_path(const std::string &p_path, PathOperation p_operation, const SymlinkMap &p_symlinks = {});
bool validate_symlink_target(const std::string &p_target, std::string &r_error);

} // namespace go
