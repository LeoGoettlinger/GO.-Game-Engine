// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace go {

std::string fallback_commit_message();
bool looks_like_conventional_commit(const std::string &p_subject);
bool looks_like_secret_file(const std::string &p_path);
std::string sanitize_commit_message(const std::string &p_raw);
std::string truncate_diff(const std::string &p_diff, size_t p_max_chars, const std::vector<std::string> &p_changed_files = {});
std::string build_commit_prompt(const std::string &p_status, const std::string &p_diff_stat, const std::string &p_diff);

} // namespace go
