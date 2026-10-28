// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_git_lfs.h"

#include "go_suite_core.h"

#include <algorithm>
#include <sstream>

namespace go {

namespace {

const std::vector<std::string> LFS_PATTERNS = {
	"*.png",
	"*.jpg",
	"*.jpeg",
	"*.tga",
	"*.bmp",
	"*.psd",
	"*.aseprite",
	"*.ttf",
	"*.otf",
	"*.woff2",
	"*.glb",
	"*.bin",
	"*.fbx",
	"*.blend",
	"*.obj",
	"*.dae",
	"*.wav",
	"*.ogg",
	"*.mp3",
	"*.mp4",
	"*.webm",
	"*.zip",
	"*.imported",
	"*.tres.bin",
};

const std::vector<std::string> TEXT_FORMATS = {
	"gd",
	"tscn",
	"tres",
	"gltf",
	"import",
	"uid",
	"svg",
	"json",
	"cfg",
	"godot",
	"md",
	"txt",
	"ini",
	"xml",
	"yml",
	"yaml",
	"csv",
	"shader",
	"gdshader",
	"gdshaderinc",
	"cs",
	"cpp",
	"h",
	"py",
	"html",
	"css",
};

constexpr uint64_t MIB = 1024ULL * 1024ULL;
const std::string BEGIN_MARKER = "# GO. Git LFS (automatisch verwaltet)";
const std::string END_MARKER = "# GO. Git LFS block end";

bool wildcard_match(const std::string &p_pattern, const std::string &p_text, size_t p_pattern_index, size_t p_text_index) {
	if (p_pattern_index == p_pattern.size()) {
		return p_text_index == p_text.size();
	}
	const char pattern_char = p_pattern[p_pattern_index];
	if (pattern_char == '*') {
		for (size_t i = p_text_index; i <= p_text.size(); ++i) {
			if (wildcard_match(p_pattern, p_text, p_pattern_index + 1, i)) {
				return true;
			}
		}
		return false;
	}
	if (p_text_index == p_text.size()) {
		return false;
	}
	if (pattern_char == '?' || pattern_char == p_text[p_text_index]) {
		return wildcard_match(p_pattern, p_text, p_pattern_index + 1, p_text_index + 1);
	}
	return false;
}

} // namespace

bool valid_lfs_folder(const std::string &p_folder) {
	if (p_folder.empty() || p_folder.front() == '/' || p_folder.back() == '/' || p_folder.find_first_of("\\:*?[]\"\n\r\t") != std::string::npos) {
		return false;
	}
	for (const std::string &part : split(p_folder, '/')) {
		if (part.empty() || part.front() == '.' || part == "export" || part == "exports") {
			return false;
		}
	}
	return true;
}

std::string lfs_literal_pattern(const std::string &p_path) {
	// Quotes protect whitespace; escaped wildcards keep file names literal.
	std::string result = "\"/";
	for (char c : p_path) {
		if (c == '*' || c == '?' || c == '[' || c == ']') {
			result += "\\\\";
		}
		if (c == '\\' || c == '\"') {
			result += '\\';
		}
		result += c;
	}
	return result + "\"";
}

std::vector<std::string> lfs_folder_patterns(const std::vector<std::string> &p_folders, bool p_text_exclusions) {
	std::vector<std::string> patterns;
	for (const std::string &folder : p_folders) {
		if (!valid_lfs_folder(folder)) {
			continue;
		}
		if (!p_text_exclusions) {
			patterns.push_back("\"/" + folder + "/**\"");
			continue;
		}
		std::vector<std::string> excluded_extensions = TEXT_FORMATS;
		excluded_extensions.insert(excluded_extensions.end(), { "pck", "exe", "apk", "aab" });
		for (const std::string &extension : excluded_extensions) {
			std::string case_pattern;
			for (char c : extension) {
				case_pattern += "[";
				case_pattern += c;
				case_pattern += char(c - 'a' + 'A');
				case_pattern += "]";
			}
			patterns.push_back("\"/" + folder + "/**/*." + case_pattern + "\"");
		}
		for (const std::string &directory : std::vector<std::string>{ ".git", ".godot", "export", "exports" }) {
			patterns.push_back("\"/" + folder + "/**/" + directory + "/**\"");
		}
	}
	return patterns;
}

bool lfs_policy_selects(const LfsPolicy &p_policy, const std::string &p_path, uint64_t p_size) {
	const std::string lower_path = lower(p_path);
	if (!valid_lfs_folder(p_path) || is_text_format(p_path) || ends_with(lower_path, ".pck") || ends_with(lower_path, ".exe") || ends_with(lower_path, ".apk") || ends_with(lower_path, ".aab")) {
		return false;
	}
	if (p_policy.mode == LfsMode::SIZE) {
		return p_policy.threshold_mib >= 1 && p_policy.threshold_mib <= 1048576 && p_size >= p_policy.threshold_mib * MIB;
	}
	if (p_policy.mode == LfsMode::FOLDERS) {
		for (const std::string &folder : p_policy.folders) {
			if (valid_lfs_folder(folder) && starts_with(p_path, folder + "/")) {
				return true;
			}
		}
		return false;
	}
	return is_lfs_pattern(p_path);
}

const std::vector<std::string> &default_lfs_patterns() {
	return LFS_PATTERNS;
}

const std::vector<std::string> &default_text_formats() {
	return TEXT_FORMATS;
}

std::string gitattributes_begin_marker() {
	return BEGIN_MARKER;
}

std::string gitattributes_end_marker() {
	return END_MARKER;
}

std::string make_gitattributes_block(const std::vector<std::string> &p_patterns) {
	std::string result = BEGIN_MARKER + "\n";
	for (const std::string &pattern : p_patterns) {
		result += pattern + " filter=lfs diff=lfs merge=lfs -text\n";
	}
	result += END_MARKER + "\n";
	return result;
}

std::string merge_gitattributes(const std::string &p_existing, const std::vector<std::string> &p_patterns, const std::vector<std::string> &p_exclusions) {
	const std::string newline = contains(p_existing, "\r\n") ? "\r\n" : "\n";
	std::string normalized = p_existing;
	size_t crlf = normalized.find("\r\n");
	while (crlf != std::string::npos) {
		normalized.replace(crlf, 2, "\n");
		crlf = normalized.find("\r\n", crlf + 1);
	}
	std::vector<std::string> lines = split(normalized, '\n');
	while (!lines.empty() && trim(lines.back()).empty()) {
		lines.pop_back();
	}

	size_t begin = lines.size();
	for (size_t i = 0; i < lines.size(); ++i) {
		if (lines[i] == BEGIN_MARKER) {
			begin = i;
			break;
		}
	}
	if (begin < lines.size()) {
		size_t end = lines.size();
		for (size_t i = begin + 1; i < lines.size(); ++i) {
			if (lines[i] == END_MARKER) {
				end = i + 1;
				break;
			}
		}
		if (end == lines.size() && (lines.empty() || lines.back() != END_MARKER)) {
			// Legacy block without an end marker: consume LFS-looking lines only.
			end = begin + 1;
			while (end < lines.size() && (starts_with(lines[end], "*") || starts_with(lines[end], "# GO. Git LFS"))) {
				end++;
			}
		}
		lines.erase(lines.begin() + begin, lines.begin() + end);
	}

	std::string block = make_gitattributes_block(p_patterns);
	std::string exclusions;
	for (const std::string &pattern : p_exclusions) {
		exclusions += pattern + " !filter !diff !merge !text\n";
	}
	block.insert(block.find(END_MARKER), exclusions);
	std::vector<std::string> block_lines = split(block, '\n');
	while (!block_lines.empty() && block_lines.back().empty()) {
		block_lines.pop_back();
	}
	lines.insert(lines.begin() + begin, block_lines.begin(), block_lines.end());

	std::string result = join(lines, newline);
	if (!result.empty()) {
		result += newline;
	}
	return result;
}

bool match_glob(const std::string &p_pattern, const std::string &p_path) {
	if (starts_with(p_pattern, "*.")) {
		const std::string suffix = p_pattern.substr(1); // includes the leading dot
		if (suffix.find_first_of("/*?") == std::string::npos) {
			return ends_with(lower(p_path), lower(suffix));
		}
	}
	return wildcard_match(lower(p_pattern), lower(p_path), 0, 0);
}

bool is_lfs_pattern(const std::string &p_path, const std::vector<std::string> &p_patterns) {
	for (const std::string &pattern : p_patterns) {
		if (match_glob(pattern, p_path)) {
			return true;
		}
	}
	return false;
}

bool is_text_format(const std::string &p_path) {
	const std::string extension = file_extension(p_path);
	if (extension.empty()) {
		const std::string basename = lower(file_basename(p_path));
		return basename == ".gitignore" || basename == ".gitattributes" || basename == ".editorconfig" || basename == "license" || basename == "readme";
	}
	for (const std::string &format : TEXT_FORMATS) {
		if (extension == format) {
			return true;
		}
	}
	return false;
}

std::string suggested_pattern(const std::string &p_path) {
	const std::string extension = file_extension(p_path);
	if (!extension.empty()) {
		return "*." + extension;
	}
	return p_path;
}

bool is_lfs_pointer_prefix(const std::string &p_file_start) {
	return starts_with(p_file_start, "version https://git-lfs.github.com/spec/v1");
}

LargeFileDecision classify_large_file(const std::string &p_path, uint64_t p_size_bytes, uint64_t p_warning_mib, GitHost p_host, const std::vector<std::string> &p_patterns) {
	LargeFileDecision decision;
	decision.lfs_covered = is_lfs_pattern(p_path, p_patterns);
	const uint64_t warning_bytes = p_warning_mib * MIB;
	const uint64_t red_bytes = 50ULL * MIB;
	const uint64_t github_limit_bytes = GITHUB_HARD_FILE_LIMIT_MIB * MIB;

	if (is_text_format(p_path)) {
		if (p_size_bytes > warning_bytes) {
			decision.action = LargeFileAction::WARN_TEXT;
			decision.reason = "text file is larger than the configured warning threshold";
		}
		return decision;
	}

	if (!decision.lfs_covered) {
		if (p_host == GitHost::GITHUB && p_size_bytes >= github_limit_bytes) {
			decision.action = LargeFileAction::BLOCK;
			decision.hard_limit = true;
			decision.reason = "GitHub rejects files of 100 MiB or more without Git LFS";
		} else if (p_size_bytes > red_bytes) {
			decision.action = LargeFileAction::BLOCK;
			decision.reason = "binary file above 50 MiB is not covered by a Git LFS rule";
		} else {
			decision.action = LargeFileAction::TRACK_LFS;
			decision.reason = "binary asset is not covered by a Git LFS rule";
		}
		return decision;
	}

	if (p_size_bytes > red_bytes) {
		decision.action = LargeFileAction::WARN_QUOTA;
		decision.reason = "large LFS file may use remote quota";
	} else if (p_size_bytes > warning_bytes) {
		decision.action = LargeFileAction::WARN;
		decision.reason = "file is larger than the configured warning threshold";
	}
	return decision;
}

std::string quota_warning(GitHost p_host, uint64_t p_total_lfs_bytes) {
	if (p_host == GitHost::GITHUB && p_total_lfs_bytes >= GITHUB_HARD_FILE_LIMIT_MIB * MIB) {
		return "GitHub files without LFS are rejected from 100 MiB; use the managed LFS block.";
	}
	if (p_host == GitHost::CODEBERG && p_total_lfs_bytes >= CODEBERG_LFS_STORAGE_GUIDANCE_MIB * MIB) {
		return "Codeberg LFS quota guidance is about 1.5 GiB; a large push may fail.";
	}
	if (p_host == GitHost::CODEBERG && p_total_lfs_bytes >= CODEBERG_GIT_STORAGE_GUIDANCE_MIB * MIB) {
		return "Codeberg recommends keeping Git repositories below about 750 MiB; consider LFS or cleanup.";
	}
	return "";
}

std::string large_file_action_name(LargeFileAction p_action) {
	switch (p_action) {
		case LargeFileAction::NONE:
			return "none";
		case LargeFileAction::TRACK_LFS:
			return "track-lfs";
		case LargeFileAction::WARN:
			return "warn";
		case LargeFileAction::WARN_TEXT:
			return "warn-text";
		case LargeFileAction::WARN_QUOTA:
			return "warn-quota";
		case LargeFileAction::BLOCK:
			return "block";
	}
	return "unknown";
}

} // namespace go
