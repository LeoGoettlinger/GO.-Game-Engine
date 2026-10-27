/**************************************************************************/
/*  go_git_remote.h                                                       */
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

namespace go {

enum class GitHost {
	NONE,
	GITHUB,
	CODEBERG,
	GITLAB,
	CUSTOM,
	LOCAL,
};

struct GitRemoteInfo {
	std::string raw_url;
	std::string host;
	std::string port;
	std::string owner;
	std::string repo;
	std::string protocol;
	std::string web_url;
	std::string normalized_url;
	std::string error;
	GitHost host_kind = GitHost::NONE;
	bool valid = false;
};

GitRemoteInfo parse_remote(const std::string &p_url);
std::string build_remote_url(GitHost p_host, const std::string &p_owner, const std::string &p_repo, const std::string &p_protocol);
std::string build_remote_url(const std::string &p_host, const std::string &p_owner, const std::string &p_repo, const std::string &p_protocol);
std::string build_web_url(GitHost p_host, const std::string &p_host_name, const std::string &p_owner, const std::string &p_repo);
std::string host_name(GitHost p_host);
std::string host_display(GitHost p_host);
std::string host_create_url(GitHost p_host);
std::string host_ssh_docs_url(GitHost p_host);
std::string host_lfs_docs_url(GitHost p_host);
GitHost classify_host(const std::string &p_host);
bool is_safe_remote_url(const std::string &p_url);
bool is_valid_repo_component(const std::string &p_component, bool p_allow_slash);

constexpr uint64_t GITHUB_HARD_FILE_LIMIT_MIB = 100;
constexpr uint64_t CODEBERG_GIT_STORAGE_GUIDANCE_MIB = 750;
constexpr uint64_t CODEBERG_LFS_STORAGE_GUIDANCE_MIB = 1536;

} // namespace go
