// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT
//
// Local-only probe for the pure Git/LFS command builder and parsers. It is
// compiled by go_git_flow_test.py and never contacts a network service.

#include "go_git_lfs.h"
#include "go_git_remote.h"
#include "go_suite_core.h"
#include "go_system_git.h"

#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

std::string read_stdin() {
	return std::string(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
}

go::GitHost parse_host(const std::string &p_name) {
	const std::string name = go::lower(go::trim(p_name));
	if (name == "github") {
		return go::GitHost::GITHUB;
	}
	if (name == "codeberg") {
		return go::GitHost::CODEBERG;
	}
	if (name == "gitlab") {
		return go::GitHost::GITLAB;
	}
	if (name == "local") {
		return go::GitHost::LOCAL;
	}
	return go::GitHost::CUSTOM;
}

void print_command(const go::GitCommand &p_command) {
	if (!p_command.valid) {
		std::cout << "INVALID " << p_command.error << std::endl;
		return;
	}
	std::cout << "EXEC " << p_command.executable << std::endl;
	for (size_t i = 0; i < p_command.arguments.size(); ++i) {
		std::cout << "ARG " << i << " " << p_command.arguments[i] << std::endl;
	}
	std::cout << "DISPLAY " << p_command.display << std::endl;
}

std::vector<std::string> tail_args(int argc, char **argv, int p_start) {
	std::vector<std::string> result;
	for (int i = p_start; i < argc; ++i) {
		result.push_back(argv[i]);
	}
	return result;
}

} // namespace

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "usage: go_git_probe <command> [args]" << std::endl;
		return 2;
	}
	const std::string mode = argv[1];

	if (mode == "cmd") {
		if (argc < 4) {
			std::cerr << "usage: go_git_probe cmd <project> <operation> [args]" << std::endl;
			return 2;
		}
		const std::string project = argv[2];
		const std::string operation = argv[3];
		const std::vector<std::string> extra = tail_args(argc, argv, 4);
		go::GitCommand result;
		if (operation == "status") {
			result = go::status(project);
		} else if (operation == "diff-stat") {
			result = go::diff_stat(project);
		} else if (operation == "log") {
			result = go::log(project, 25);
		} else if (operation == "branch") {
			result = go::branch_show_current(project);
		} else if (operation == "repo-check") {
			result = go::rev_parse_verify_repo(project);
		} else if (operation == "remote-get") {
			result = go::remote_get_url(project, extra.empty() ? "origin" : extra[0]);
		} else if (operation == "fetch") {
			result = go::fetch(project);
		} else if (operation == "pull") {
			result = go::pull(project, "origin", extra.empty() ? "" : extra[0]);
		} else if (operation == "push") {
			result = go::push(project, "origin", extra.empty() ? "" : extra[0], !extra.empty());
		} else if (operation == "add-all") {
			result = go::add_all(project);
		} else if (operation == "add-paths") {
			result = go::add_paths(project, extra);
		} else if (operation == "commit") {
			result = go::commit(project, extra.empty() ? "chore: update project files" : extra[0]);
		} else if (operation == "remote-add") {
			result = go::remote_add(project, "origin", extra.empty() ? "" : extra[0]);
		} else if (operation == "remote-set") {
			result = go::remote_set_url(project, "origin", extra.empty() ? "" : extra[0]);
		} else if (operation == "lfs-version") {
			result = go::lfs_version();
		} else if (operation == "lfs-install-local") {
			result = go::lfs_install_local(project);
		} else if (operation == "lfs-track") {
			result = go::lfs_track(project, extra.empty() ? "*.png" : extra[0]);
		} else if (operation == "lfs-pull") {
			result = go::lfs_pull(project);
		} else if (operation == "lfs-push") {
			result = go::lfs_push(project);
		} else if (operation == "lfs-ls") {
			result = go::lfs_ls_files_size(project);
		} else if (operation == "lfs-status") {
			result = go::lfs_status(project);
		} else if (operation == "lfs-env") {
			result = go::lfs_env(project);
		} else if (operation == "install-linux") {
			result = go::lfs_install_linux();
		} else if (operation == "install-windows") {
			result = go::lfs_install_windows();
		} else if (operation == "lfs-migrate") {
			result = go::lfs_migrate_import(project, extra.empty() ? std::vector<std::string>{ "*.png" } : extra);
		} else {
			std::cerr << "unknown operation: " << operation << std::endl;
			return 2;
		}
		print_command(result);
		return result.valid ? 0 : 1;
	}

	if (mode == "parse-status") {
		const go::GitStatus status = go::parse_status_porcelain(read_stdin());
		std::cout << "BRANCH " << status.branch << std::endl;
		std::cout << "AHEAD " << status.ahead << std::endl;
		std::cout << "BEHIND " << status.behind << std::endl;
		std::cout << "CHANGES " << (status.has_changes ? 1 : 0) << std::endl;
		std::cout << "CONFLICT " << (status.merge_conflict ? 1 : 0) << std::endl;
		std::cout << "ENTRIES " << status.entries.size() << std::endl;
		return 0;
	}

	if (mode == "parse-remote") {
		if (argc < 3) {
			return 2;
		}
		const go::GitRemoteInfo info = go::parse_remote(argv[2]);
		std::cout << "VALID " << (info.valid ? 1 : 0) << std::endl;
		std::cout << "HOST " << go::host_name(info.host_kind) << std::endl;
		std::cout << "OWNER " << info.owner << std::endl;
		std::cout << "REPO " << info.repo << std::endl;
		std::cout << "PROTOCOL " << info.protocol << std::endl;
		std::cout << "WEB " << info.web_url << std::endl;
		std::cout << "SAFE " << (go::is_safe_remote_url(argv[2]) ? 1 : 0) << std::endl;
		return 0;
	}

	if (mode == "folder-attributes") {
		std::vector<std::string> folders;
		for (int i = 2; i < argc; ++i) {
			if (!go::valid_lfs_folder(argv[i])) {
				return 2;
			}
			folders.push_back(argv[i]);
		}
		std::cout << go::merge_gitattributes(read_stdin(), go::lfs_folder_patterns(folders), go::lfs_folder_patterns(folders, true));
		return 0;
	}

	if (mode == "attributes") {
		std::cout << go::merge_gitattributes(read_stdin());
		return 0;
	}

	if (mode == "large") {
		if (argc < 6) {
			std::cerr << "usage: go_git_probe large <path> <bytes> <warn_mib> <host>" << std::endl;
			return 2;
		}
		const go::LargeFileDecision decision = go::classify_large_file(
				argv[2], std::strtoull(argv[3], nullptr, 10), std::strtoull(argv[4], nullptr, 10), parse_host(argv[5]));
		std::cout << "ACTION " << go::large_file_action_name(decision.action) << std::endl;
		std::cout << "COVERED " << (decision.lfs_covered ? 1 : 0) << std::endl;
		std::cout << "HARD " << (decision.hard_limit ? 1 : 0) << std::endl;
		std::cout << "PATTERN " << go::suggested_pattern(argv[2]) << std::endl;
		return 0;
	}

	if (mode == "pointer") {
		std::cout << "POINTER " << (go::is_lfs_pointer_content(read_stdin()) ? 1 : 0) << std::endl;
		return 0;
	}

	if (mode == "parse-lfs-ls") {
		const go::LfsSnapshot snapshot = go::parse_lfs_ls_files(read_stdin());
		std::cout << "FILES " << snapshot.files.size() << std::endl;
		std::cout << "TOTAL " << snapshot.total_bytes << std::endl;
		std::cout << "LARGEST " << snapshot.largest.size() << std::endl;
		if (!snapshot.largest.empty()) {
			std::cout << "FIRST " << snapshot.largest.front().path << std::endl;
		}
		return 0;
	}

	if (mode == "aheadbehind") {
		int ahead = 0;
		int behind = 0;
		const bool ok = go::parse_ahead_behind(read_stdin(), ahead, behind);
		std::cout << "OK " << (ok ? 1 : 0) << std::endl;
		std::cout << "AHEAD " << ahead << std::endl;
		std::cout << "BEHIND " << behind << std::endl;
		return ok ? 0 : 1;
	}

	std::cerr << "unknown mode: " << mode << std::endl;
	return 2;
}
