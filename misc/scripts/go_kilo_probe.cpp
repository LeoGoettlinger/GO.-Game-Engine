/**************************************************************************/
/*  go_kilo_probe.cpp                                                     */
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

// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT
//
// Local-only Kilo Gateway model-filter probe used by go_mock_gateway_test.py.
// It speaks plain HTTP to 127.0.0.1 and never contacts the real gateway.

#include "go_kilo_models.h"
#include "go_suite_core.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct HttpResponse {
	int status = 0;
	std::vector<std::string> headers;
	std::string body;
};

bool http_request(const std::string &p_host, int p_port, const std::string &p_path, const std::string &p_method, const std::string &p_body, HttpResponse &r_response, std::string &r_error) {
	const int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		r_error = "socket() failed";
		return false;
	}
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(static_cast<uint16_t>(p_port));
	if (inet_pton(AF_INET, p_host.c_str(), &address.sin_addr) != 1) {
		r_error = "inet_pton() failed for " + p_host;
		close(sock);
		return false;
	}
	if (connect(sock, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
		r_error = "connect() failed";
		close(sock);
		return false;
	}
	std::string request = p_method + " " + p_path + " HTTP/1.1\r\n"
													"Host: " +
			p_host + ":" + std::to_string(p_port) + "\r\n"
													"Connection: close\r\n"
													"Accept: application/json\r\n";
	if (!p_body.empty()) {
		request += "Content-Type: application/json\r\nContent-Length: " + std::to_string(p_body.size()) + "\r\n";
	}
	request += "\r\n" + p_body;
	size_t sent = 0;
	while (sent < request.size()) {
		const ssize_t result = send(sock, request.data() + sent, request.size() - sent, 0);
		if (result <= 0) {
			r_error = "send() failed";
			close(sock);
			return false;
		}
		sent += static_cast<size_t>(result);
	}
	std::string raw;
	char buffer[4096];
	while (true) {
		const ssize_t received = recv(sock, buffer, sizeof(buffer), 0);
		if (received <= 0) {
			break;
		}
		raw.append(buffer, static_cast<size_t>(received));
	}
	close(sock);
	const size_t header_end = raw.find("\r\n\r\n");
	if (header_end == std::string::npos) {
		r_error = "malformed HTTP response";
		return false;
	}
	const std::string header_block = raw.substr(0, header_end);
	const std::vector<std::string> lines = go::split(header_block, '\n');
	if (lines.empty()) {
		r_error = "empty HTTP response";
		return false;
	}
	const std::vector<std::string> status_parts = go::split(go::trim(lines[0]), ' ');
	if (status_parts.size() < 2) {
		r_error = "malformed HTTP status line";
		return false;
	}
	r_response.status = std::atoi(status_parts[1].c_str());
	for (size_t i = 1; i < lines.size(); ++i) {
		const std::string header = go::trim(lines[i]);
		if (!header.empty() && header.back() == '\r') {
			r_response.headers.push_back(header.substr(0, header.size() - 1));
		} else if (!header.empty()) {
			r_response.headers.push_back(header);
		}
	}
	r_response.body = raw.substr(header_end + 4);
	r_error.clear();
	return true;
}

} // namespace

int main(int argc, char **argv) {
	if (argc < 2 || !go::starts_with(std::string(argv[1]), "http://")) {
		std::cerr << "usage: go_kilo_probe http://127.0.0.1:PORT" << std::endl;
		return 2;
	}
	std::string authority = std::string(argv[1]).substr(7);
	const size_t slash = authority.find('/');
	if (slash != std::string::npos) {
		authority = authority.substr(0, slash);
	}
	const size_t colon = authority.find(':');
	if (colon == std::string::npos) {
		std::cerr << "port is required" << std::endl;
		return 2;
	}
	const std::string host = authority.substr(0, colon);
	const int port = std::atoi(authority.substr(colon + 1).c_str());

	HttpResponse models_response;
	std::string error;
	if (!http_request(host, port, "/models", "GET", "", models_response, error) || models_response.status != 200) {
		std::cerr << "GET /models failed: " << error << std::endl;
		return 1;
	}
	std::string parse_error;
	const std::vector<go::GoKiloModel> models = go::parse_models_response(models_response.body, parse_error);
	if (!parse_error.empty()) {
		std::cerr << "parse /models failed: " << parse_error << std::endl;
		return 1;
	}
	const std::vector<go::GoKiloModel> free_models = go::free_models_for_picker(models);
	std::cout << "MODELS_STATUS " << models_response.status << std::endl;
	for (const go::GoKiloModel &model : free_models) {
		std::cout << "MODEL " << model.id << std::endl;
	}

	const std::string chat_request = go::build_chat_request_json(
			go::DEFAULT_FREE_MODEL,
			{ { "system", "Use only whitelisted tools." }, { "user", "List files" } },
			true,
			"[{\"name\":\"list_files\"}]");
	HttpResponse chat_response;
	if (!http_request(host, port, "/chat/completions", "POST", chat_request, chat_response, error) || chat_response.status != 200) {
		std::cerr << "POST /chat/completions failed: " << error << std::endl;
		return 1;
	}
	std::string chat_error;
	const go::GoKiloChatResponse chat = go::parse_chat_response(chat_response.body, chat_error);
	if (!chat_error.empty() || !chat.error.empty()) {
		std::cerr << "parse /chat/completions failed: " << chat_error << chat.error << std::endl;
		return 1;
	}
	const std::vector<go::GoKiloToolCall> chat_calls = go::parse_tool_calls_json(chat.tool_calls_json);
	std::cout << "CHAT_STATUS " << chat_response.status << std::endl;
	std::cout << "CHAT_CONTENT " << chat.content << std::endl;
	for (const go::GoKiloToolCall &call : chat_calls) {
		std::cout << "CHAT_TOOL " << call.name << std::endl;
	}

	HttpResponse rate_response;
	if (!http_request(host, port, "/rate-limit", "GET", "", rate_response, error)) {
		std::cerr << "GET /rate-limit failed: " << error << std::endl;
		return 1;
	}
	const go::KiloHttpError rate = go::parse_http_error(rate_response.status, rate_response.headers, rate_response.body);
	std::cout << "RATE_LIMIT status=" << rate.status << " retry=" << rate.retry_after_seconds
			  << " message=" << rate.message << std::endl;
	return 0;
}
