// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_git_remote.h"

#include "go_suite_core.h"

#include <algorithm>
#include <cctype>

namespace go {

namespace {

std::string strip_query_and_fragment(const std::string &p_text) {
	const size_t query = p_text.find('?');
	const size_t fragment = p_text.find('#');
	size_t end = std::min(query == std::string::npos ? p_text.size() : query,
			fragment == std::string::npos ? p_text.size() : fragment);
	return p_text.substr(0, end);
}

bool contains_credential_marker(const std::string &p_url) {
	const std::string value = lower(p_url);
	for (const char *marker : { "token=", "password=", "passwd=", "api_key=", "apikey=", "secret=", "access_token=" }) {
		if (contains(value, marker)) {
			return true;
		}
	}
	return false;
}

std::vector<std::string> path_segments(const std::string &p_path) {
	std::vector<std::string> result;
	std::string cleaned = strip_query_and_fragment(p_path);
	while (!cleaned.empty() && cleaned.front() == '/') {
		cleaned.erase(cleaned.begin());
	}
	while (!cleaned.empty() && cleaned.back() == '/') {
		cleaned.pop_back();
	}
	for (const std::string &part : split(cleaned, '/')) {
		if (!part.empty() && part != ".") {
			result.push_back(part);
		}
	}
	return result;
}

void split_host_port(const std::string &p_authority, std::string &r_host, std::string &r_port) {
	std::string authority = p_authority;
	const size_t userinfo = authority.rfind('@');
	if (userinfo != std::string::npos) {
		authority = authority.substr(userinfo + 1);
	}
	if (!authority.empty() && authority.front() == '[') {
		const size_t bracket = authority.find(']');
		if (bracket != std::string::npos) {
			r_host = authority.substr(1, bracket - 1);
			if (bracket + 1 < authority.size() && authority[bracket + 1] == ':') {
				r_port = authority.substr(bracket + 2);
			}
			return;
		}
	}
	const size_t colon = authority.rfind(':');
	if (colon != std::string::npos && authority.find(':') == colon) {
		r_host = authority.substr(0, colon);
		r_port = authority.substr(colon + 1);
	} else {
		r_host = authority;
		r_port.clear();
	}
}

std::string strip_dot_git_case_insensitive(std::string p_value) {
	if (p_value.size() >= 4 && lower(p_value.substr(p_value.size() - 4)) == ".git") {
		return p_value.substr(0, p_value.size() - 4);
	}
	return p_value;
}

std::string web_url_for(const std::string &p_scheme, const std::string &p_host, const std::string &p_port, const std::vector<std::string> &p_segments) {
	if (p_host.empty() || p_segments.empty()) {
		return "";
	}
	std::vector<std::string> segments = p_segments;
	segments.back() = strip_dot_git_case_insensitive(segments.back());
	std::string authority = p_host;
	if (!p_port.empty() && p_port != "443" && p_port != "80") {
		authority += ":" + p_port;
	}
	return p_scheme + "://" + authority + "/" + join(segments, "/");
}

bool has_embedded_password(const std::string &p_authority) {
	const size_t at = p_authority.find('@');
	if (at == std::string::npos) {
		return false;
	}
	return p_authority.substr(0, at).find(':') != std::string::npos;
}

void fill_owner_repo(GitRemoteInfo &r_info, const std::vector<std::string> &p_segments) {
	if (p_segments.empty()) {
		r_info.error = "remote URL has no repository path";
		return;
	}
	r_info.repo = strip_dot_git_case_insensitive(p_segments.back());
	if (p_segments.size() > 1) {
		std::vector<std::string> owner_parts(p_segments.begin(), p_segments.end() - 1);
		r_info.owner = join(owner_parts, "/");
	}
	r_info.valid = true;
}

} // namespace

GitHost classify_host(const std::string &p_host) {
	const std::string host = lower(trim(p_host));
	if (host == "github.com" || ends_with(host, ".github.com")) {
		return GitHost::GITHUB;
	}
	if (host == "codeberg.org" || ends_with(host, ".codeberg.org")) {
		return GitHost::CODEBERG;
	}
	if (host == "gitlab.com" || ends_with(host, ".gitlab.com")) {
		return GitHost::GITLAB;
	}
	return GitHost::CUSTOM;
}

std::string host_name(GitHost p_host) {
	switch (p_host) {
		case GitHost::GITHUB:
			return "github";
		case GitHost::CODEBERG:
			return "codeberg";
		case GitHost::GITLAB:
			return "gitlab";
		case GitHost::CUSTOM:
			return "custom";
		case GitHost::LOCAL:
			return "local";
		case GitHost::NONE:
		default:
			return "none";
	}
}

std::string host_display(GitHost p_host) {
	switch (p_host) {
		case GitHost::GITHUB:
			return "GitHub";
		case GitHost::CODEBERG:
			return "Codeberg";
		case GitHost::GITLAB:
			return "GitLab";
		case GitHost::CUSTOM:
			return "Custom";
		case GitHost::LOCAL:
			return "Local";
		case GitHost::NONE:
		default:
			return "Only local";
	}
}

std::string host_create_url(GitHost p_host) {
	switch (p_host) {
		case GitHost::GITHUB:
			return "https://github.com/new";
		case GitHost::CODEBERG:
			return "https://codeberg.org/repo/create";
		case GitHost::GITLAB:
			return "https://gitlab.com/projects/new";
		default:
			return "";
	}
}

std::string host_ssh_docs_url(GitHost p_host) {
	switch (p_host) {
		case GitHost::GITHUB:
			return "https://docs.github.com/en/authentication/connecting-to-github-with-ssh";
		case GitHost::CODEBERG:
			return "https://docs.codeberg.org/security/ssh-key/";
		case GitHost::GITLAB:
			return "https://docs.gitlab.com/user/ssh/";
		default:
			return "";
	}
}

std::string host_lfs_docs_url(GitHost p_host) {
	switch (p_host) {
		case GitHost::GITHUB:
			return "https://docs.github.com/en/repositories/working-with-files/managing-large-files/configuring-git-large-file-storage";
		case GitHost::CODEBERG:
			return "https://docs.codeberg.org/git/using-lfs/";
		case GitHost::GITLAB:
			return "https://docs.gitlab.com/topics/git/lfs/";
		default:
			return "";
	}
}

GitRemoteInfo parse_remote(const std::string &p_url) {
	GitRemoteInfo info;
	info.raw_url = p_url;
	const std::string url = trim(p_url);
	if (url.empty()) {
		info.host_kind = GitHost::NONE;
		info.error = "no remote configured";
		return info;
	}
	if (contains_credential_marker(url)) {
		info.error = "remote URLs must not contain tokens, passwords or query credentials";
		return info;
	}
	if (url.find('\0') != std::string::npos) {
		info.error = "remote URL contains invalid characters";
		return info;
	}
	info.normalized_url = strip_query_and_fragment(url);

	const std::string lowered = lower(url);
	if (starts_with(lowered, "file://")) {
		info.host_kind = GitHost::LOCAL;
		info.protocol = "file";
		info.repo = strip_dot_git_case_insensitive(file_basename(url.substr(7)));
		info.web_url.clear();
		info.valid = true;
		return info;
	}
	if (starts_with(url, "/") || starts_with(url, "\\\\") || starts_with(url, "./") || starts_with(url, "../") ||
			(url.size() >= 3 && std::isalpha(static_cast<unsigned char>(url[0])) && url[1] == ':' &&
					(url[2] == '/' || url[2] == '\\'))) {
		info.host_kind = GitHost::LOCAL;
		info.protocol = "local";
		info.repo = strip_dot_git_case_insensitive(file_basename(url));
		info.valid = true;
		return info;
	}

	if (starts_with(lowered, "https://") || starts_with(lowered, "http://") || starts_with(lowered, "ssh://")) {
		const size_t scheme_end = url.find("://") + 3;
		const std::string scheme = lower(url.substr(0, scheme_end - 3));
		const std::string rest = url.substr(scheme_end);
		const size_t slash = rest.find('/');
		if (slash == std::string::npos) {
			info.error = "remote URL has no path";
			return info;
		}
		const std::string authority = rest.substr(0, slash);
		if (has_embedded_password(authority)) {
			info.error = "embedded passwords are not allowed; use the system Git credential helper";
			return info;
		}
		split_host_port(authority, info.host, info.port);
		info.host = lower(info.host);
		info.protocol = scheme == "ssh" ? "ssh" : scheme;
		const std::vector<std::string> segments = path_segments(rest.substr(slash + 1));
		fill_owner_repo(info, segments);
		if (!info.valid) {
			return info;
		}
		info.host_kind = classify_host(info.host);
		if (info.host_kind == GitHost::GITHUB || info.host_kind == GitHost::CODEBERG || info.host_kind == GitHost::GITLAB) {
			info.web_url = "https://" + info.host + "/" + (info.owner.empty() ? "" : info.owner + "/") + info.repo;
		} else if (scheme == "http" || scheme == "https") {
			info.web_url = web_url_for(scheme, info.host, info.port, segments);
		} else if (contains(info.host, ".")) {
			info.web_url = web_url_for("https", info.host, info.port, segments);
		}
		return info;
	}

	// SCP-like SSH syntax: git@host:owner/repo.git
	const size_t at = url.find('@');
	if (at != std::string::npos) {
		const size_t colon = url.find(':', at + 1);
		if (colon != std::string::npos) {
			const std::string userinfo = url.substr(0, at);
			if (contains(userinfo, ":")) {
				info.error = "embedded passwords are not allowed";
				return info;
			}
			info.host = lower(url.substr(at + 1, colon - at - 1));
			info.protocol = "ssh";
			const std::vector<std::string> segments = path_segments(url.substr(colon + 1));
			fill_owner_repo(info, segments);
			if (!info.valid) {
				return info;
			}
			info.host_kind = classify_host(info.host);
			if (info.host_kind == GitHost::GITHUB || info.host_kind == GitHost::CODEBERG || info.host_kind == GitHost::GITLAB) {
				info.web_url = "https://" + info.host + "/" + (info.owner.empty() ? "" : info.owner + "/") + info.repo;
			} else if (contains(info.host, ".")) {
				info.web_url = web_url_for("https", info.host, info.port, segments);
			}
			return info;
		}
	}

	if (contains(url, "://")) {
		const size_t scheme_end = url.find("://");
		info.protocol = lower(url.substr(0, scheme_end));
		info.host_kind = GitHost::CUSTOM;
		const std::vector<std::string> segments = path_segments(url.substr(scheme_end + 3));
		fill_owner_repo(info, segments);
		return info;
	}

	info.host_kind = GitHost::LOCAL;
	info.protocol = "local";
	info.repo = strip_dot_git_case_insensitive(file_basename(url));
	info.valid = true;
	return info;
}

bool is_valid_repo_component(const std::string &p_component, bool p_allow_slash) {
	if (p_component.empty() || p_component.size() > 128) {
		return false;
	}
	if (contains(p_component, "..") || contains(p_component, "://") || contains(p_component, "@")) {
		return false;
	}
	for (char c : p_component) {
		const unsigned char uc = static_cast<unsigned char>(c);
		if (std::isalnum(uc) || c == '-' || c == '_' || c == '.' || (p_allow_slash && c == '/')) {
			continue;
		}
		return false;
	}
	return true;
}

std::string build_remote_url(GitHost p_host, const std::string &p_owner, const std::string &p_repo, const std::string &p_protocol) {
	switch (p_host) {
		case GitHost::GITHUB:
			return build_remote_url("github.com", p_owner, p_repo, p_protocol);
		case GitHost::CODEBERG:
			return build_remote_url("codeberg.org", p_owner, p_repo, p_protocol);
		case GitHost::GITLAB:
			return build_remote_url("gitlab.com", p_owner, p_repo, p_protocol);
		default:
			return "";
	}
}

std::string build_remote_url(const std::string &p_host, const std::string &p_owner, const std::string &p_repo, const std::string &p_protocol) {
	const std::string host = lower(trim(p_host));
	const std::string owner = trim(p_owner);
	const std::string repo = strip_dot_git_case_insensitive(trim(p_repo));
	const std::string protocol = lower(trim(p_protocol));
	if (host.empty() || !is_valid_repo_component(owner, true) || !is_valid_repo_component(repo, false)) {
		return "";
	}
	if (protocol == "https" || protocol == "http") {
		return protocol + "://" + host + "/" + owner + "/" + repo + ".git";
	}
	if (protocol == "ssh") {
		return "git@" + host + ":" + owner + "/" + repo + ".git";
	}
	return "";
}

std::string build_web_url(GitHost p_host, const std::string &p_host_name, const std::string &p_owner, const std::string &p_repo) {
	(void)p_host;
	const std::string host = lower(trim(p_host_name));
	const std::string owner = trim(p_owner);
	const std::string repo = strip_dot_git_case_insensitive(trim(p_repo));
	if (host.empty() || !is_valid_repo_component(owner, true) || !is_valid_repo_component(repo, false)) {
		return "";
	}
	return "https://" + host + "/" + owner + "/" + repo;
}

bool is_safe_remote_url(const std::string &p_url) {
	const GitRemoteInfo info = parse_remote(p_url);
	return info.valid && info.error.empty();
}

} // namespace go
