/**************************************************************************/
/*  go_auto_commit.h                                                      */
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

#pragma once

// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

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
