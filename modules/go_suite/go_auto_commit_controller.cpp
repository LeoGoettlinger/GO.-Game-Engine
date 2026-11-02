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
	if (p_what == NOTIFICATION_PROCESS && running && git_phase != GitPhase::NONE && git_process.poll()) {
		_git_finished();
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
		GoSuiteRegistry::set_git_job_busy(false);
		if (SceneTree::get_singleton() != nullptr && SceneTree::get_singleton()->get_root() != nullptr) {
			SceneTree::get_singleton()->get_root()->set_disable_input(previous_input_disabled);
		}
		owns_lock = false;
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
	git_phase = GitPhase::NONE;
	git_process.cancel();
	kilo_client->cancel_request();
	set_process(false);
	if (state.is_active()) {
		if (p_success) {
			state.finish();
		} else {
			state.fail(go_suite::to_std_string(p_message));
		}
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
				const std::string prompt = go::build_commit_prompt(status_text, diff_stat, diff_text);
				const String system_prompt = TTR("You write git commit messages. Output only a Conventional Commit subject and optional short bullet body. Summarize what changed and why.");
				kilo_client->request_chat_simple(String::utf8(go::DEFAULT_FREE_MODEL), go_suite::to_godot_string(prompt), system_prompt);
				_update_overlay();
				return;
			}
			case go::AutoCommitStage::STAGE_FILES: {
				std::string error;
				if (!git_controller->ensure_lfs_attributes(error)) {
					_finish(false, TTR("Preparing LFS rules failed: ") + go_suite::to_godot_string(error));
					return;
				}
				_launch_git(GitPhase::STAGE_ADD, go::add_all(git_controller->get_project_path()));
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
	if (!git_process.succeeded()) {
		const String prefix = state.current_stage() == go::AutoCommitStage::PUSH ? TTR("Commit exists locally, but push failed. Inspect the repository before retrying: ") : TTR("Git operation failed: ");
		_finish(false, prefix + git_process.get_output());
		return;
	}
	const std::string output = go_suite::to_std_string(git_process.get_stdout());
	const std::string &project = git_controller->get_project_path();
	if (completed == GitPhase::STAGE_ADD) {
		_launch_git(GitPhase::STAGE_RENORMALIZE, go::git(project, { "add", "--renormalize", "--", "." }));
		return;
	}
	if (completed == GitPhase::STAGE_RENORMALIZE) {
		std::string error;
		if (!git_controller->verify_staged_lfs(error)) {
			_finish(false, TTR("LFS pointer verification failed: ") + go_suite::to_godot_string(error));
			return;
		}
		_append_details(TTR("Files staged and LFS pointers verified."));
		state.advance();
		_update_overlay();
		_advance();
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
	std::string error;
	const go_suite::GoGitSnapshot snapshot = git_controller->snapshot(error);
	go::AutoCommitPreflight preflight;
	preflight.is_repo = snapshot.is_repo;
	preflight.has_changes = snapshot.status.has_changes;
	preflight.merge_conflict = snapshot.status.merge_conflict;
	preflight.user_name_set = !snapshot.user_name.empty();
	preflight.user_email_set = !snapshot.user_email.empty();
	preflight.lfs_ready = snapshot.lfs_ready || (snapshot.lfs.files.empty() && snapshot.suggested_lfs_patterns.empty());
	if (!snapshot.is_repo) {
		preflight.error = error.empty() ? "This project is not a Git repository." : error;
	}

	std::ostringstream status_stream;
	status_stream << "branch=" << snapshot.branch
				  << " ahead=" << snapshot.status.ahead
				  << " behind=" << snapshot.status.behind
				  << " files=" << snapshot.status.entries.size();
	for (size_t i = 0; i < snapshot.status.entries.size() && i < 64; ++i) {
		status_stream << "\n"
					  << snapshot.status.entries[i].path.substr(0, 256);
	}
	status_text = status_stream.str();

	state = go::AutoCommitStateMachine(go::make_auto_commit_plan(snapshot.remote.valid, false, !snapshot.suggested_lfs_patterns.empty()));
	if (!state.begin(preflight)) {
		emit_signal("finished", false, go_suite::to_godot_string(state.error()));
		return false;
	}

	running = true;
	commit_message.clear();
	command_output.clear();
	kilo_used = false;
	kilo_fallback = false;
	if (details != nullptr) {
		details->set_text("");
	}
	_append_details(TTR("Preflight passed. The editor is locked until this operation finishes."));
	if (!_show_overlay()) {
		_finish(false, TTR("Could not acquire the editor input lock."));
		return false;
	}
	_update_overlay();
	_advance();
	return true;
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
