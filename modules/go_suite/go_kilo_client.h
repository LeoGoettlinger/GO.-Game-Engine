// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "go_kilo_models.h"

#include "scene/main/node.h"

#include <vector>

class GoKiloClient : public Node {
	GDCLASS(GoKiloClient, Node);

protected:
	static void _bind_methods();

public:
	String gateway_url() const;
	String models_url() const;
	String chat_url() const;
	String anonymous_key() const;
	String user_agent() const;
	std::vector<go::GoKiloModel> offline_models() const;
};
