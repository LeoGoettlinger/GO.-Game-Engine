// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>

namespace go {

enum class AutoCommitStage {
	PREFLIGHT,
	READ_CHANGES,
	GENERATE_MESSAGE,
	STAGE_FILES,
	CREATE_COMMIT,
	PUSH,
	DONE,
	FAILED,
};

struct AutoCommitPreflight {
	bool is_repo = false;
	bool has_changes = false;
	bool merge_conflict = false;
	bool user_name_set = false;
	bool user_email_set = false;
	bool lfs_ready = true;
	std::string error;
};

struct AutoCommitPlan {
	std::vector<AutoCommitStage> stages;
	bool will_push = false;
	bool needs_lfs_fix = false;
};

class AutoCommitStateMachine {
	AutoCommitPlan plan;
	size_t stage_index = 0;
	bool active = false;
	bool locked = false;
	std::string error_text;

public:
	explicit AutoCommitStateMachine(const AutoCommitPlan &p_plan = AutoCommitPlan());

	bool begin(const AutoCommitPreflight &p_preflight);
	bool advance();
	bool fail(const std::string &p_error);
	bool finish();

	bool is_active() const { return active; }
	bool is_locked() const { return locked; }
	bool invariant_holds() const { return active == locked; }
	AutoCommitStage current_stage() const;
	const std::string &error() const { return error_text; }
	const AutoCommitPlan &get_plan() const { return plan; }
};

bool preflight_ok(const AutoCommitPreflight &p_preflight, std::string &r_error);
AutoCommitPlan make_auto_commit_plan(bool p_has_remote, bool p_has_upstream, bool p_lfs_needed);
std::string stage_label(AutoCommitStage p_stage);
std::string stage_name(AutoCommitStage p_stage);

} // namespace go
