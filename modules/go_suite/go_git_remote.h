// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

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
