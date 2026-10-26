// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT
//
// Local-only Kilo Gateway model-filter probe used by go_mock_gateway_test.py.
// It speaks plain HTTP to 127.0.0.1 and never contacts the real gateway.

#include "go_kilo_models.h"
#include "go_suite_core.h"

#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

struct HttpResponse {
	int status = 0;
	std::vector<std::string> headers;
	std::string body;
};

bool http_get(const std::string &p_host, int p_port, const std::string &p_path, HttpResponse &r_response, std::string &r_error) {
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
	const std::string request =
			"GET " + p_path + " HTTP/1.1\r\n"
			"Host: " + p_host + ":" + std::to_string(p_port) + "\r\n"
			"Connection: close\r\n"
			"Accept: application/json\r\n\r\n";
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
	if (!http_get(host, port, "/models", models_response, error) || models_response.status != 200) {
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

	HttpResponse rate_response;
	if (!http_get(host, port, "/rate-limit", rate_response, error)) {
		std::cerr << "GET /rate-limit failed: " << error << std::endl;
		return 1;
	}
	const go::KiloHttpError rate = go::parse_http_error(rate_response.status, rate_response.headers, rate_response.body);
	std::cout << "RATE_LIMIT status=" << rate.status << " retry=" << rate.retry_after_seconds
			  << " message=" << rate.message << std::endl;
	return 0;
}
