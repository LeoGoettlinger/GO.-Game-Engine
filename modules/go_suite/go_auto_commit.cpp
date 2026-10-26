// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_auto_commit.h"

namespace go {

AutoCommitStateMachine::AutoCommitStateMachine(const AutoCommitPlan &p_plan) :
		plan(p_plan) {
	if (plan.stages.empty()) {
		plan = make_auto_commit_plan(false, false, false);
	}
}

bool AutoCommitStateMachine::begin(const AutoCommitPreflight &p_preflight) {
	std::string error;
	if (!preflight_ok(p_preflight, error)) {
		error_text = error;
		active = false;
		locked = false;
		return false;
	}
	error_text.clear();
	stage_index = 0;
	active = true;
	locked = true; // The UI lock is acquired in the same successful transition as activation.
	return true;
}

bool AutoCommitStateMachine::advance() {
	if (!active) {
		return false;
	}
	if (stage_index + 1 >= plan.stages.size()) {
		return finish();
	}
	stage_index++;
	return true;
}

bool AutoCommitStateMachine::fail(const std::string &p_error) {
	error_text = p_error.empty() ? "Automatic Commit failed." : p_error;
	active = false;
	locked = false; // Guaranteed unlock on every failure path.
	return false;
}

bool AutoCommitStateMachine::finish() {
	active = false;
	locked = false; // Guaranteed unlock on success.
	error_text.clear();
	stage_index = plan.stages.size();
	return true;
}

AutoCommitStage AutoCommitStateMachine::current_stage() const {
	if (!error_text.empty()) {
		return AutoCommitStage::FAILED;
	}
	if (!active) {
		return stage_index >= plan.stages.size() ? AutoCommitStage::DONE : AutoCommitStage::PREFLIGHT;
	}
	return plan.stages[stage_index];
}

bool preflight_ok(const AutoCommitPreflight &p_preflight, std::string &r_error) {
	if (!p_preflight.error.empty()) {
		r_error = p_preflight.error;
		return false;
	}
	if (!p_preflight.is_repo) {
		r_error = "This project is not a Git repository.";
		return false;
	}
	if (!p_preflight.has_changes) {
		r_error = "No changes to commit.";
		return false;
	}
	if (p_preflight.merge_conflict) {
		r_error = "Resolve merge conflicts before Automatic Commit.";
		return false;
	}
	if (!p_preflight.user_name_set || !p_preflight.user_email_set) {
		r_error = "Git user.name and user.email must be configured.";
		return false;
	}
	if (!p_preflight.lfs_ready) {
		r_error = "Git LFS is required for this repository but is not ready.";
		return false;
	}
	r_error.clear();
	return true;
}

AutoCommitPlan make_auto_commit_plan(bool p_has_remote, bool p_has_upstream, bool p_lfs_needed) {
	(void)p_has_upstream; // Upstream existence is consumed by the Git runner's push arguments.
	AutoCommitPlan plan;
	plan.stages = {
		AutoCommitStage::PREFLIGHT,
		AutoCommitStage::READ_CHANGES,
		AutoCommitStage::GENERATE_MESSAGE,
		AutoCommitStage::STAGE_FILES,
		AutoCommitStage::CREATE_COMMIT,
	};
	plan.will_push = p_has_remote;
	plan.needs_lfs_fix = p_lfs_needed;
	if (p_has_remote) {
		plan.stages.push_back(AutoCommitStage::PUSH);
	}
	return plan;
}

std::string stage_label(AutoCommitStage p_stage) {
	switch (p_stage) {
		case AutoCommitStage::PREFLIGHT:
			return "Checking repository and Git LFS";
		case AutoCommitStage::READ_CHANGES:
			return "Reading status and diff";
		case AutoCommitStage::GENERATE_MESSAGE:
			return "Generating commit message with kilo-auto/free";
		case AutoCommitStage::STAGE_FILES:
			return "Preparing Git LFS and staging files";
		case AutoCommitStage::CREATE_COMMIT:
			return "Creating commit";
		case AutoCommitStage::PUSH:
			return "Pushing to remote";
		case AutoCommitStage::DONE:
			return "Done";
		case AutoCommitStage::FAILED:
			return "Failed";
	}
	return "Unknown";
}

std::string stage_name(AutoCommitStage p_stage) {
	switch (p_stage) {
		case AutoCommitStage::PREFLIGHT:
			return "preflight";
		case AutoCommitStage::READ_CHANGES:
			return "read_changes";
		case AutoCommitStage::GENERATE_MESSAGE:
			return "generate_message";
		case AutoCommitStage::STAGE_FILES:
			return "stage_files";
		case AutoCommitStage::CREATE_COMMIT:
			return "create_commit";
		case AutoCommitStage::PUSH:
			return "push";
		case AutoCommitStage::DONE:
			return "done";
		case AutoCommitStage::FAILED:
			return "failed";
	}
	return "unknown";
}

} // namespace go
