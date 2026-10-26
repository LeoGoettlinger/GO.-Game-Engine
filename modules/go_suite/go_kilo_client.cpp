// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_kilo_client.h"

#include "core/string/ustring.h"

void GoKiloClient::_bind_methods() {
}

String GoKiloClient::gateway_url() const {
	return String::utf8(go::DEFAULT_KILO_GATEWAY_URL);
}

String GoKiloClient::models_url() const {
	return String::utf8(go::models_endpoint(go::DEFAULT_KILO_GATEWAY_URL).c_str());
}

String GoKiloClient::chat_url() const {
	return String::utf8(go::chat_endpoint(go::DEFAULT_KILO_GATEWAY_URL).c_str());
}

String GoKiloClient::anonymous_key() const {
	return String::utf8(go::ANONYMOUS_API_KEY);
}

String GoKiloClient::user_agent() const {
	return String::utf8(go::USER_AGENT_BASE) + "/GO. Game Engine";
}

std::vector<go::GoKiloModel> GoKiloClient::offline_models() const {
	return go::free_models_for_picker({});
}
