/**************************************************************************/
/*  go_suite_core.h                                                       */
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
