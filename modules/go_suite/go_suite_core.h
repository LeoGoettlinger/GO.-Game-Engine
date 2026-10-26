// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace go {

std::string lower(std::string p_text);
std::string upper(std::string p_text);
std::string trim(const std::string &p_text);
bool starts_with(const std::string &p_text, const std::string &p_prefix);
bool ends_with(const std::string &p_text, const std::string &p_suffix);
bool contains(const std::string &p_text, const std::string &p_needle);
std::vector<std::string> split(const std::string &p_text, char p_separator);
std::string join(const std::vector<std::string> &p_parts, const std::string &p_separator);
std::string strip_suffix(const std::string &p_text, const std::string &p_suffix);
std::string strip_prefix(const std::string &p_text, const std::string &p_prefix);
std::string file_extension(const std::string &p_path);
std::string file_basename(const std::string &p_path);
std::string path_dirname(const std::string &p_path);
std::string human_size(uint64_t p_bytes);

} // namespace go
