/**************************************************************************/
/*  go_auto_commit_controller.cpp                                         */
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

#include "go_auto_commit_controller.h"

#include "go_commit_message.h"
#include "go_kilo_client.h"
#include "go_kilo_models.h"
#include "go_suite_core.h"
#include "go_suite_registry.h"
#include "go_system_git_runner.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/string/ustring.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/center_container.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/text_edit.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

#include <algorithm>
#include <cctype>
#include <sstream>

GoAutoCommitController *GoAutoCommitController::singleton = nullptr;

namespace {

String stage_text(go::AutoCommitStage p_stage) {
	switch (p_stage) {
		case go::AutoCommitStage::PREFLIGHT:
			return TTR("Checking repository and Git LFS…");
		case go::AutoCommitStage::READ_CHANGES:
			return TTR("Reading changes and diff…");
		case go::AutoCommitStage::GENERATE_MESSAGE:
			return TTR("Generating commit message with kilo-auto/free…");
		case go::AutoCommitStage::STAGE_FILES:
			return TTR("Preparing Git LFS and staging files…");
		case go::AutoCommitStage::CREATE_COMMIT:
			return TTR("Creating commit…");
		case go::AutoCommitStage::PUSH:
			return TTR("Pushing to remote (including Git LFS objects)…");
		case go::AutoCommitStage::DONE:
			return TTR("Finished.");
		case go::AutoCommitStage::FAILED:
			return TTR("Failed.");
	}
	return TTR("Working…");
}

std::string first_line(const std::string &p_text) {
	const size_t end = p_text.find('\n');
	return end == std::string::npos ? p_text : p_text.substr(0, end);
}

std::string commit_hash_from_output(const std::string &p_output) {
	const size_t open = p_output.find('[');
	if (open == std::string::npos) {
		return "";
	}
	const size_t close = p_output.find(']', open);
	if (close == std::string::npos || close <= open + 1) {
		return "";
	}
	const std::string inside = p_output.substr(open + 1, close - open - 1);
	const size_t space = inside.rfind(' ');
	const std::string token = space == std::string::npos ? inside : inside.substr(space + 1);
	if (token.size() < 7 || token.size() > 40) {
		return "";
	}
	for (const char c : token) {
		if (!std::isxdigit(static_cast<unsigned char>(c))) {
			return "";
		}
	}
	return token;
}

} // namespace

void GoAutoCommitController::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start"), &GoAutoCommitController::start);
	ClassDB::bind_method(D_METHOD("is_running"), &GoAutoCommitController::is_running);
	ADD_SIGNAL(MethodInfo("finished", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::STRING, "message")));
}

void GoAutoCommitController::_notification(int p_what) {
	if (p_what == NOTIFICATION_PROCESS && running) {
		if (preparing_files) {
			_prepare_lfs_files();
		} else if (git_phase != GitPhase::NONE && git_process.poll()) {
			_git_finished();
		}
	}
	if (p_what == NOTIFICATION_EXIT_TREE || p_what == NOTIFICATION_PREDELETE) {
		running = false;
		git_process.cancel();
		if (kilo_client != nullptr) {
			kilo_client->cancel_request();
		}
		state.fail("Automatic Commit was interrupted while the editor was closing.");
		_hide_overlay();
		_dispose_overlay();
		if (singleton == this) {
			singleton = nullptr;
		}
	}
}

Control *GoAutoCommitController::_overlay() const {
	return Object::cast_to<Control>(ObjectDB::get_instance(overlay_id));
}

void GoAutoCommitController::_dispose_overlay() {
	Control *overlay = _overlay();
	if (overlay != nullptr && !overlay->is_queued_for_deletion()) {
		overlay->queue_free();
	}
	overlay_id = ObjectID();
}

void GoAutoCommitController::_build_ui() {
	Control *overlay = memnew(Control);
	overlay_id = overlay->get_instance_id();
	overlay->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	overlay->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	overlay->set_focus_mode(Control::FOCUS_ALL);
	add_child(overlay);

	shade = memnew(ColorRect);
	shade->set_color(Color(0.015, 0.02, 0.035, 0.74));
	shade->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	shade->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	overlay->add_child(shade);

	CenterContainer *center = memnew(CenterContainer);
	center->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	overlay->add_child(center);

	panel = memnew(PanelContainer);
	panel->set_custom_minimum_size(Size2(620, 300) * EDSCALE);
	center->add_child(panel);

	VBoxContainer *root = memnew(VBoxContainer);
	root->set_theme_type_variation("PanelContainer");
	panel->add_child(root);

	Label *title = memnew(Label);
	title->set_text(TTR("Automatic Commit running…"));
	title->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	root->add_child(title);

	status_label = memnew(Label);
	status_label->set_text(TTR("Starting…"));
	status_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	root->add_child(status_label);

	progress = memnew(ProgressBar);
	progress->set_min(0.0);
	progress->set_max(100.0);
	progress->set_show_percentage(false);
	progress->set_indeterminate(true);
	progress->set_custom_minimum_size(Size2(0, 18) * EDSCALE);
	root->add_child(progress);

	details = memnew(TextEdit);
	details->set_editable(false);
	details->set_placeholder(TTR("Git and Kilo output appears here."));
	details->set_custom_minimum_size(Size2(0, 120) * EDSCALE);
	details->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(details);

	hide();
}

bool GoAutoCommitController::_show_overlay() {
	Control *overlay = _overlay();
	if (overlay == nullptr || GoSuiteRegistry::is_editor_input_locked()) {
		return false;
	}
	Node *parent = nullptr;
	if (EditorNode::get_singleton() != nullptr && EditorNode::get_singleton()->get_gui_base() != nullptr) {
		parent = EditorNode::get_singleton()->get_gui_base();
	} else if (SceneTree::get_singleton() != nullptr) {
		parent = SceneTree::get_singleton()->get_root();
	}
	if (parent == nullptr || SceneTree::get_singleton() == nullptr || SceneTree::get_singleton()->get_root() == nullptr) {
		return false;
	}
	// Reparent only the presentation, never the running controller. Moving the
	// controller itself triggers EXIT_TREE and used to cancel its own operation.
	if (overlay->get_parent() != parent) {
		overlay->reparent(parent);
	}
	overlay->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	overlay->set_z_index(1000);
	overlay->show();
	overlay->grab_focus();
	owns_lock = true;
	owns_job = true;
	GoSuiteRegistry::set_git_job_busy(true);
	GoSuiteRegistry::set_editor_input_locked(true);
	if (SceneTree::get_singleton() != nullptr && SceneTree::get_singleton()->get_root() != nullptr) {
		Window *root = SceneTree::get_singleton()->get_root();
		previous_input_disabled = root->is_input_disabled();
		root->set_disable_input(true);
	}
	return true;
}

void GoAutoCommitController::_hide_overlay() {
	if (owns_lock) {
		GoSuiteRegistry::set_editor_input_locked(false);
		if (SceneTree::get_singleton() != nullptr && SceneTree::get_singleton()->get_root() != nullptr) {
			SceneTree::get_singleton()->get_root()->set_disable_input(previous_input_disabled);
		}
		owns_lock = false;
	}
	if (owns_job) {
		GoSuiteRegistry::set_git_job_busy(false);
		owns_job = false;
	}
	if (Control *overlay = _overlay()) {
		overlay->hide();
	}
}

void GoAutoCommitController::_update_overlay() {
	if (status_label != nullptr) {
		status_label->set_text(stage_text(state.current_stage()));
	}
	if (progress != nullptr) {
		const go::AutoCommitPlan &plan = state.get_plan();
		size_t index = 0;
		for (size_t i = 0; i < plan.stages.size(); ++i) {
			if (plan.stages[i] == state.current_stage()) {
				index = i;
				break;
			}
		}
		const double value = plan.stages.empty() ? 0.0 : (static_cast<double>(index) / static_cast<double>(plan.stages.size())) * 100.0;
		progress->set_value(value);
		progress->set_indeterminate(state.current_stage() == go::AutoCommitStage::GENERATE_MESSAGE || state.current_stage() == go::AutoCommitStage::PUSH);
	}
}

void GoAutoCommitController::_append_details(const String &p_text) {
	if (details == nullptr || p_text.is_empty()) {
		return;
	}
	details->set_text(details->get_text() + p_text + "\n");
}

void GoAutoCommitController::_finish(bool p_success, const String &p_message) {
	if (!running) {
		return;
	}
	running = false;
	preparing_files = false;
	last_success = p_success;
	last_result = p_message;
	git_phase = GitPhase::NONE;
	git_process.cancel();
	kilo_client->cancel_request();
	set_process(false);
	if (state.is_active() && p_success) {
		state.finish();
	}
	if (!p_success) {
		state.fail(go_suite::to_std_string(p_message));
	}
	_hide_overlay();
	emit_signal("finished", p_success, p_message);
}

void GoAutoCommitController::_advance() {
	if (!running || git_controller == nullptr) {
		_finish(false, TTR("The Git controller is not available."));
		return;
	}

	while (running && state.is_active()) {
		switch (state.current_stage()) {
			case go::AutoCommitStage::PREFLIGHT: {
				state.advance();
			} break;
			case go::AutoCommitStage::READ_CHANGES: {
				diff_stat.clear();
				diff_text.clear();
				_launch_git(GitPhase::STAT_WORKTREE, go::diff_stat(git_controller->get_project_path()));
				return;
			}
			case go::AutoCommitStage::GENERATE_MESSAGE: {
#ifdef TESTS_ENABLED
				if (test_message_fallback) {
					_on_kilo_failed(TTR("Offline test fixture."));
					return;
				}
#endif
				const std::string prompt = go::build_commit_prompt(status_text, diff_stat, diff_text);
				const String system_prompt = TTR("You write git commit messages. Output only a Conventional Commit subject and optional short bullet body. Summarize what changed and why.");
				kilo_client->request_chat_simple(String::utf8(go::DEFAULT_FREE_MODEL), go_suite::to_godot_string(prompt), system_prompt);
				_update_overlay();
				return;
			}
			case go::AutoCommitStage::STAGE_FILES: {
				if (lfs_enabled) {
					_launch_git(GitPhase::LFS_INSTALL, go::lfs_install_local(git_controller->get_project_path()));
				} else {
					_launch_git(GitPhase::STAGE_ADD, go::add_all(git_controller->get_project_path()));
				}
				return;
			}
			case go::AutoCommitStage::CREATE_COMMIT: {
				_launch_git(GitPhase::COMMIT, go::commit(git_controller->get_project_path(), commit_message));
				return;
			}
			case go::AutoCommitStage::PUSH: {
				_launch_git(GitPhase::BRANCH, go::branch_show_current(git_controller->get_project_path()));
				return;
			}
			case go::AutoCommitStage::DONE:
			case go::AutoCommitStage::FAILED: {
				_finish(state.current_stage() == go::AutoCommitStage::DONE, state.current_stage() == go::AutoCommitStage::DONE ? TTR("Automatic Commit finished.") : TTR("Automatic Commit failed."));
				return;
			}
		}
		_update_overlay();
	}

	if (!state.is_active()) {
		String summary = TTR("Automatic Commit finished.");
		const std::string hash = commit_hash_from_output(command_output);
		if (!hash.empty()) {
			summary += " " + go_suite::to_godot_string(hash);
		}
		summary += " — " + go_suite::to_godot_string(first_line(commit_message));
		if (kilo_fallback) {
			summary += " (" + TTR("Kilo was unavailable; fallback message used") + ")";
		} else if (kilo_used) {
			summary += " (" + TTR("Kilo kilo-auto/free message used") + ")";
		}
		if (!state.get_plan().will_push) {
			summary += " " + TTR("No remote configured — not pushed.");
		}
		_finish(true, summary);
	}
}

void GoAutoCommitController::_launch_git(GitPhase p_phase, const go::GitCommand &p_command) {
	git_phase = p_phase;
	_update_overlay();
	if (!git_process.start(p_command)) {
		_finish(false, TTR("Could not start Git: ") + git_process.get_output());
		return;
	}
	set_process(true);
}

void GoAutoCommitController::_git_finished() {
	if (!running || git_controller == nullptr) {
		return;
	}
	const GitPhase completed = git_phase;
	git_phase = GitPhase::NONE;
	const bool missing_config = (completed == GitPhase::CHECK_REMOTE || completed == GitPhase::CHECK_NAME || completed == GitPhase::CHECK_EMAIL) && git_process.get_exit_code() == 1 && !git_process.has_process_error();
	if (git_process.is_output_truncated() || (!git_process.succeeded() && !missing_config)) {
		const String prefix = state.current_stage() == go::AutoCommitStage::PUSH ? TTR("Commit exists locally, but push failed. Inspect the repository before retrying: ") : TTR("Git operation failed or exceeded its output limit: ");
		_finish(false, prefix + git_process.get_output());
		return;
	}
	const std::string output = git_process.get_stdout_bytes();
	const std::string &project = git_controller->get_project_path();
	if (completed == GitPhase::CHECK_STATUS || completed == GitPhase::CHECK_STATUS_LOCKED) {
		std::string error;
		const go::GitStatus status = go::parse_status_porcelain_z(output, error);
		preflight.is_repo = true;
		preflight.has_changes = status.has_changes;
		preflight.merge_conflict = status.merge_conflict;
		if (!error.empty() || !status.has_changes || status.merge_conflict) {
			_finish(false, error.empty() ? (status.merge_conflict ? TTR("Resolve merge conflicts before committing.") : TTR("No changes to commit.")) : go_suite::to_godot_string(error));
			return;
		}
		status_text.clear();
		for (const auto &entry : status.entries) {
			if (status_text.size() >= 2000) {
				break;
			}
			status_text += std::string(1, entry.index_status) + entry.worktree_status + " " + entry.path + "\n";
		}
		if (completed == GitPhase::CHECK_STATUS_LOCKED) {
			_advance();
			return;
		}
		_launch_git(GitPhase::CHECK_NAME, go::git(project, { "config", "--get", "user.name" }));
		return;
	}
	if (completed == GitPhase::CHECK_NAME || completed == GitPhase::CHECK_EMAIL) {
		if (missing_config || go::trim(output).empty()) {
			_finish(false, TTR("Configure Git user.name and user.email before committing."));
			return;
		}
		if (completed == GitPhase::CHECK_NAME) {
			preflight.user_name_set = true;
			_launch_git(GitPhase::CHECK_EMAIL, go::git(project, { "config", "--get", "user.email" }));
		} else {
			preflight.user_email_set = true;
			_launch_git(GitPhase::CHECK_REMOTE, go::git(project, { "config", "--get", "remote.origin.url" }));
		}
		return;
	}
	if (completed == GitPhase::CHECK_REMOTE) {
		has_remote = !missing_config;
		if (has_remote && !go::is_safe_remote_argument(go::trim(output))) {
			_finish(false, TTR("The origin remote is not safe for Automatic Commit."));
			return;
		}
		if (lfs_enabled) {
			_launch_git(GitPhase::CHECK_LFS, go::git(project, { "lfs", "version" }));
		} else {
			_preflight_ready();
		}
		return;
	}
	if (completed == GitPhase::CHECK_LFS) {
		_preflight_ready();
		return;
	}
	if (completed == GitPhase::LFS_INSTALL) {
		if (lfs_policy.mode == go::LfsMode::SIZE) {
			_launch_git(GitPhase::LFS_FILES, go::git(project, { "ls-files", "-z", "--cached", "--others", "--exclude-standard" }));
		} else {
			if (lfs_policy.mode == go::LfsMode::FOLDERS) {
				for (const auto &folder : lfs_policy.folders) {
					if (!go::valid_lfs_folder(folder)) {
						_finish(false, TTR("Invalid saved LFS folder."));
						return;
					}
				}
				lfs_patterns = go::lfs_folder_patterns(lfs_policy.folders);
				lfs_exclusions = go::lfs_folder_patterns(lfs_policy.folders, true);
			} else {
				lfs_patterns = go::default_lfs_patterns();
			}
			_write_rules_and_stage();
		}
		return;
	}
	if (completed == GitPhase::LFS_FILES || completed == GitPhase::VERIFY_FILES) {
		file_paths.clear();
		file_index = 0;
		size_t offset = 0;
		while (offset < output.size()) {
			const size_t end = output.find('\0', offset);
			if (end == std::string::npos || end == offset || file_paths.size() >= 10000) {
				_finish(false, TTR("Invalid or excessive Git file list; no commit created."));
				return;
			}
			file_paths.push_back(output.substr(offset, end - offset));
			offset = end + 1;
		}
		if (completed == GitPhase::LFS_FILES) {
			preparing_files = true;
		} else {
			_verify_next_pointer();
		}
		return;
	}
	if (completed == GitPhase::VERIFY_ATTR) {
		const std::string prefix = file_paths[file_index] + std::string(1, '\0') + "filter" + std::string(1, '\0');
		if (output.size() <= prefix.size() || output.compare(0, prefix.size(), prefix) != 0 || output.back() != '\0') {
			_finish(false, TTR("Invalid staged attribute response; no commit created."));
			return;
		}
		if (output.substr(prefix.size()) == std::string("lfs\0", 4)) {
			_launch_git(GitPhase::VERIFY_SIZE, go::git(project, { "cat-file", "-s", ":0:" + file_paths[file_index] }));
		} else {
			++file_index;
			_verify_next_pointer();
		}
		return;
	}
	if (completed == GitPhase::VERIFY_SIZE) {
		const std::string size_text = go::trim(output);
		if (size_text.empty() || size_text.size() > 4 || size_text.find_first_not_of("0123456789") != std::string::npos || std::stoul(size_text) > 1024) {
			_finish(false, TTR("Staged LFS file is not a pointer; no commit created."));
			return;
		}
		_launch_git(GitPhase::VERIFY_POINTER, go::git(project, { "cat-file", "blob", ":0:" + file_paths[file_index] }));
		return;
	}
	if (completed == GitPhase::VERIFY_POINTER) {
		if (!go::is_lfs_pointer_content(output)) {
			_finish(false, TTR("Invalid staged LFS pointer; no commit created."));
			return;
		}
		++file_index;
		_verify_next_pointer();
		return;
	}
	if (completed == GitPhase::STAGE_ADD) {
		_launch_git(GitPhase::STAGE_RENORMALIZE, go::git(project, { "add", "--renormalize", "--", "." }));
		return;
	}
	if (completed == GitPhase::STAGE_RENORMALIZE) {
		_launch_git(GitPhase::VERIFY_FILES, go::git(project, { "diff", "--cached", "--name-only", "--diff-filter=ACMRT", "-z" }));
		return;
	}
	if (completed == GitPhase::STAT_WORKTREE || completed == GitPhase::STAT_INDEX) {
		diff_stat += output.substr(0, 3000);
		if (completed == GitPhase::STAT_WORKTREE) {
			go::GitCommand command = go::diff_stat(project);
			command.arguments.push_back("--cached");
			_launch_git(GitPhase::STAT_INDEX, command);
		} else {
			_launch_git(GitPhase::TEXT_WORKTREE, go::diff_text(project, 12000));
		}
		return;
	}
	if (completed == GitPhase::TEXT_WORKTREE || completed == GitPhase::TEXT_INDEX) {
		diff_text += go::truncate_diff(output, 12000 - diff_text.size());
		if (completed == GitPhase::TEXT_WORKTREE) {
			go::GitCommand command = go::diff_text(project, 12000);
			command.arguments.push_back("--cached");
			_launch_git(GitPhase::TEXT_INDEX, command);
			return;
		}
		_append_details(TTR("Working-tree and staged diffs read with privacy filtering."));
	} else if (completed == GitPhase::BRANCH) {
		const std::string branch = go::parse_branch_name(output);
		if (branch.empty()) {
			_finish(false, TTR("Commit exists locally. Detached HEAD cannot be pushed automatically."));
			return;
		}
		_launch_git(GitPhase::PUSH, go::push(project, "origin", branch, true));
		return;
	} else {
		command_output += output;
		_append_details(git_process.get_output());
	}
	state.advance();
	_update_overlay();
	_advance();
}

void GoAutoCommitController::_on_chat_completed(const String &p_content, const String &p_tool_calls_json, bool p_used_json_fallback) {
	(void)p_tool_calls_json;
	(void)p_used_json_fallback;
	if (!running) {
		return;
	}
	commit_message = go::sanitize_commit_message(go_suite::to_std_string(p_content));
	kilo_used = true;
	_append_details(TTR("Kilo message: ") + go_suite::to_godot_string(first_line(commit_message)));
	if (state.current_stage() == go::AutoCommitStage::GENERATE_MESSAGE) {
		state.advance();
	}
	_advance();
}

void GoAutoCommitController::_on_kilo_failed(const String &p_message) {
	if (!running) {
		return;
	}
	commit_message = go::fallback_commit_message();
	kilo_fallback = true;
	_append_details(TTR("Kilo request failed; using fallback message: ") + p_message);
	if (state.current_stage() == go::AutoCommitStage::GENERATE_MESSAGE) {
		state.advance();
	}
	_advance();
}

void GoAutoCommitController::_on_request_cancelled() {
	_on_kilo_failed(TTR("Kilo request was cancelled."));
}

bool GoAutoCommitController::start() {
	if (!is_inside_tree() || running || git_controller == nullptr || GoSuiteRegistry::is_git_job_busy() || GoSuiteRegistry::is_quit_pending() || GoSuiteRegistry::is_editor_input_locked()) {
		return false;
	}
	running = true;
	owns_job = true;
	GoSuiteRegistry::set_git_job_busy(true);
	state = go::AutoCommitStateMachine();
	preflight = go::AutoCommitPreflight();
	has_remote = false;
	lfs_enabled = bool(ProjectSettings::get_singleton()->get_setting("go/git_suite/lfs/enabled", false));
	lfs_policy = git_controller->lfs_policy();
	preparing_files = false;
	file_paths.clear();
	file_index = 0;
	lfs_patterns.clear();
	lfs_exclusions.clear();
	commit_message.clear();
	command_output.clear();
	status_text.clear();
	kilo_used = false;
	kilo_fallback = false;
	last_result = String();
	last_success = false;
	if (details != nullptr) {
		details->clear();
	}
	_launch_git(GitPhase::CHECK_STATUS, go::git(git_controller->get_project_path(), { "status", "--porcelain=v1", "-z", "--untracked-files=all" }));
	return running;
}

void GoAutoCommitController::_preflight_ready() {
	state = go::AutoCommitStateMachine(go::make_auto_commit_plan(has_remote, false, lfs_enabled));
	if (!state.begin(preflight)) {
		_finish(false, go_suite::to_godot_string(state.error()));
		return;
	}
	if (!_show_overlay()) {
		_finish(false, TTR("Could not acquire the editor input lock."));
		return;
	}
	_update_overlay();
	_launch_git(GitPhase::CHECK_STATUS_LOCKED, go::git(git_controller->get_project_path(), { "status", "--porcelain=v1", "-z", "--untracked-files=all" }));
}

void GoAutoCommitController::_prepare_lfs_files() {
	Ref<DirAccess> directory = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	const String project = go_suite::to_godot_string(git_controller->get_project_path());
	// Bounded work per frame; no Git commands execute synchronously here.
	for (int budget = 0; budget < 16 && file_index < file_paths.size(); ++budget, ++file_index) {
		const std::string &file = file_paths[file_index];
		if (file.find_first_of("\\:*?[]\"\n\r\t") != std::string::npos) {
			_finish(false, TTR("Automatic LFS requires file names without control characters, quotes or wildcards."));
			return;
		}
		if (!go::valid_lfs_folder(file)) {
			continue;
		}
		String path = project;
		bool linked = false;
		for (const auto &part : go::split(file, '/')) {
			path = path.path_join(go_suite::to_godot_string(part));
			linked = linked || directory->is_link(path);
		}
		if (linked) {
			continue;
		}
		const int64_t size = FileAccess::get_size(path);
		if (size >= 0 && go::lfs_policy_selects(lfs_policy, file, uint64_t(size))) {
			lfs_patterns.push_back(go::lfs_literal_pattern(file));
		}
	}
	if (file_index == file_paths.size()) {
		preparing_files = false;
		_write_rules_and_stage();
	}
}

void GoAutoCommitController::_write_rules_and_stage() {
	std::sort(lfs_patterns.begin(), lfs_patterns.end());
	lfs_patterns.erase(std::unique(lfs_patterns.begin(), lfs_patterns.end()), lfs_patterns.end());
	std::string error;
	if (!git_controller->write_lfs_attributes(lfs_patterns, lfs_exclusions, error)) {
		_finish(false, go_suite::to_godot_string(error));
		return;
	}
	_launch_git(GitPhase::STAGE_ADD, go::add_all(git_controller->get_project_path()));
}

void GoAutoCommitController::_verify_next_pointer() {
	if (file_index == file_paths.size()) {
		state.advance();
		_update_overlay();
		_advance();
		return;
	}
	_launch_git(GitPhase::VERIFY_ATTR, go::git(git_controller->get_project_path(), { "check-attr", "--cached", "-z", "filter", "--", file_paths[file_index] }));
}

bool GoAutoCommitController::request() {
	return singleton != nullptr && singleton->start();
}

GoAutoCommitController::GoAutoCommitController() {
	singleton = this;
	_build_ui();
	kilo_client = memnew(GoKiloClient);
	kilo_client->connect("chat_completed", callable_mp(this, &GoAutoCommitController::_on_chat_completed));
	kilo_client->connect("request_failed", callable_mp(this, &GoAutoCommitController::_on_kilo_failed));
	kilo_client->connect("request_cancelled", callable_mp(this, &GoAutoCommitController::_on_request_cancelled));
	add_child(kilo_client);
}

GoAutoCommitController::~GoAutoCommitController() {
	git_process.cancel();
	_dispose_overlay();
	if (running) {
		running = false;
		state.fail("Automatic Commit was interrupted.");
		_hide_overlay();
	}
	if (singleton == this) {
		singleton = nullptr;
	}
}
