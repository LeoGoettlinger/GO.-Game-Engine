// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_suite_core.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace go {

std::string lower(std::string p_text) {
	std::transform(p_text.begin(), p_text.end(), p_text.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return p_text;
}

std::string upper(std::string p_text) {
	std::transform(p_text.begin(), p_text.end(), p_text.begin(), [](unsigned char c) { return (char)std::toupper(c); });
	return p_text;
}

std::string trim(const std::string &p_text) {
	size_t start = 0;
	size_t end = p_text.size();
	while (start < end && std::isspace(static_cast<unsigned char>(p_text[start]))) {
		start++;
	}
	while (end > start && std::isspace(static_cast<unsigned char>(p_text[end - 1]))) {
		end--;
	}
	return p_text.substr(start, end - start);
}

bool starts_with(const std::string &p_text, const std::string &p_prefix) {
	return p_text.size() >= p_prefix.size() && p_text.compare(0, p_prefix.size(), p_prefix) == 0;
}

bool ends_with(const std::string &p_text, const std::string &p_suffix) {
	return p_text.size() >= p_suffix.size() && p_text.compare(p_text.size() - p_suffix.size(), p_suffix.size(), p_suffix) == 0;
}

bool contains(const std::string &p_text, const std::string &p_needle) {
	return p_text.find(p_needle) != std::string::npos;
}

std::vector<std::string> split(const std::string &p_text, char p_separator) {
	std::vector<std::string> result;
	std::string item;
	std::istringstream stream(p_text);
	while (std::getline(stream, item, p_separator)) {
		result.push_back(item);
	}
	return result;
}

std::string join(const std::vector<std::string> &p_parts, const std::string &p_separator) {
	std::string result;
	for (size_t i = 0; i < p_parts.size(); ++i) {
		if (i > 0) {
			result += p_separator;
		}
		result += p_parts[i];
	}
	return result;
}

std::string strip_suffix(const std::string &p_text, const std::string &p_suffix) {
	return ends_with(p_text, p_suffix) ? p_text.substr(0, p_text.size() - p_suffix.size()) : p_text;
}

std::string strip_prefix(const std::string &p_text, const std::string &p_prefix) {
	return starts_with(p_text, p_prefix) ? p_text.substr(p_prefix.size()) : p_text;
}

std::string file_extension(const std::string &p_path) {
	const size_t slash = p_path.find_last_of("/\\");
	const size_t dot = p_path.find_last_of('.');
	if (dot == std::string::npos || (slash != std::string::npos && dot < slash) || dot + 1 >= p_path.size()) {
		return "";
	}
	return lower(p_path.substr(dot + 1));
}

std::string file_basename(const std::string &p_path) {
	const size_t slash = p_path.find_last_of("/\\");
	return slash == std::string::npos ? p_path : p_path.substr(slash + 1);
}

std::string path_dirname(const std::string &p_path) {
	const size_t slash = p_path.find_last_of("/\\");
	return slash == std::string::npos ? "" : p_path.substr(0, slash);
}

std::string human_size(uint64_t p_bytes) {
	static const char *units[] = { "B", "KiB", "MiB", "GiB", "TiB" };
	double value = static_cast<double>(p_bytes);
	size_t unit = 0;
	while (value >= 1024.0 && unit + 1 < sizeof(units) / sizeof(units[0])) {
		value /= 1024.0;
		unit++;
	}
	std::ostringstream stream;
	stream.setf(std::ios::fixed);
	stream.precision(unit == 0 ? 0 : 1);
	stream << value << ' ' << units[unit];
	return stream.str();
}

} // namespace go
