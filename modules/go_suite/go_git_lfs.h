/**************************************************************************/
/*  go_git_lfs.h                                                          */
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

#include "go_git_remote.h"

#include <cstdint>
#include <string>
#include <vector>

namespace go {

enum class LargeFileAction {
	NONE,
	TRACK_LFS,
	WARN,
	WARN_TEXT,
	WARN_QUOTA,
	BLOCK,
};

struct LargeFileDecision {
	LargeFileAction action = LargeFileAction::NONE;
	std::string reason;
	bool lfs_covered = false;
	bool hard_limit = false;
};

const std::vector<std::string> &default_lfs_patterns();
const std::vector<std::string> &default_text_formats();
std::string gitattributes_begin_marker();
std::string gitattributes_end_marker();
std::string make_gitattributes_block(const std::vector<std::string> &p_patterns = default_lfs_patterns());
std::string merge_gitattributes(const std::string &p_existing, const std::vector<std::string> &p_patterns = default_lfs_patterns());
bool match_glob(const std::string &p_pattern, const std::string &p_path);
bool is_lfs_pattern(const std::string &p_path, const std::vector<std::string> &p_patterns = default_lfs_patterns());
bool is_text_format(const std::string &p_path);
std::string suggested_pattern(const std::string &p_path);
bool is_lfs_pointer_prefix(const std::string &p_file_start);
LargeFileDecision classify_large_file(const std::string &p_path, uint64_t p_size_bytes, uint64_t p_warning_mib, GitHost p_host, const std::vector<std::string> &p_patterns = default_lfs_patterns());
std::string quota_warning(GitHost p_host, uint64_t p_total_lfs_bytes);
std::string large_file_action_name(LargeFileAction p_action);

} // namespace go
