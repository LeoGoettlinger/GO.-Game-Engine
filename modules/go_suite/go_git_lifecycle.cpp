/**************************************************************************/
/*  go_git_lifecycle.cpp                                                  */
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

#include "go_git_lifecycle.h"

#include "go_suite_registry.h"
#include "go_system_git_runner.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/callable_mp.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "servers/display/display_server.h"

bool GoGitLifecycle::_setting(const String &p_name, bool p_default) const {
	return ProjectSettings::get_singleton()->get_setting("go/git_suite/" + p_name, p_default);
}

bool GoGitLifecycle::_save_setting(const String &p_name, bool p_value) {
	ProjectSettings *settings = ProjectSettings::get_singleton();
	const String key = "go/git_suite/" + p_name;
	const Variant previous = settings->get_setting(key);
	settings->set_setting(key, p_value);
	if (settings->save() != OK) {
		settings->set_setting(key, previous);
		dialog->set_text(TTR("Could not save the project reminder setting. The previous setting was retained."));
		dialog->popup_centered();
		return false;
	}
	return true;
}

void GoGitLifecycle::_release() {
	if (owns_job) {
		GoSuiteRegistry::set_git_job_busy(false);
		owns_job = false;
	}
	phase = Phase::IDLE;
}

void GoGitLifecycle::_launch(Phase p_phase, const go::GitCommand &p_command) {
	phase = p_phase;
	if (!process.start(p_command, 30000)) {
		status_failed = true;
		_release();
		_show_result();
	}
}

void GoGitLifecycle::_poll() {
	if (phase == Phase::IDLE || !process.poll()) {
		return;
	}
	if (phase == Phase::REMOTE) {
		if (process.succeeded() && !process.get_stdout().strip_edges().is_empty()) {
			_launch(Phase::FETCH, go::fetch(project));
		} else {
			_launch(Phase::STATUS, go::status(project));
		}
		return;
	}
	if (phase == Phase::FETCH) {
		if (!process.succeeded()) {
			print_verbose("GO. startup fetch did not finish; checking local status only.");
		}
		_launch(Phase::STATUS, go::status(project));
		return;
	}
	if (phase == Phase::STATUS) {
		status_failed = !process.succeeded();
		if (!status_failed) {
			status = go::parse_status_porcelain(go_suite::to_std_string(process.get_stdout()));
		}
		if (closing && !status_failed) {
			_launch(Phase::LFS, go::lfs_status(project));
			return;
		}
	} else if (phase == Phase::LFS && process.succeeded()) {
		int staged = 0;
		go::parse_lfs_status(go_suite::to_std_string(process.get_stdout()), staged, pending_lfs);
	}
	_release();
	_show_result();
}

void GoGitLifecycle::_show_result() {
	if (!go::git_reminder_needed(status, pending_lfs, closing, status_failed)) {
		if (closing) {
			_continue_quit();
		}
		return;
	}
	dont_remind->set_pressed(false);
	git_suite_button->set_visible(closing);
	pull_button->set_visible(!closing && !status_failed && status.behind > 0);
	dialog->get_ok_button()->set_disabled(false);
	dialog->get_ok_button()->set_text(closing ? TTR("Close Anyway") : TTR("Open Git Suite"));
	dialog->get_cancel_button()->set_text(closing ? TTR("Cancel") : TTR("Later"));
	String message = status_failed ? TTR("Git status could not be checked. Inspect the repository before closing.") : vformat(TTR("Uncommitted changes: %d\nKnown unpushed commits: %d\nCommits behind the known remote: %d\nKnown pending LFS objects: %d"), status.entries.size(), status.ahead, status.behind, pending_lfs);
	if (closing && GoSuiteRegistry::is_git_job_busy()) {
		message += "\n" + TTR("A Git operation is still running. Closing may interrupt it; completed remote changes will not be rolled back.");
	}
	message += "\n\n" + TTR("Remote counts reflect the last successful fetch. Unknown LFS state is not proof of upload. No commit, pull or push happens automatically.");
	dialog->set_text(message);
	dialog->popup_centered(Size2i(620, 300));
}

void GoGitLifecycle::_continue_quit() {
	const Callable callback = resume_quit;
	resume_quit = Callable();
	closing = false;
	// Bypass only this accepted continuation of upstream save/discard decisions.
	dialog->hide();
	allow_close = true;
	if (callback.is_valid()) {
		callback.call();
	}
	allow_close = false;
	GoSuiteRegistry::set_quit_pending(false);
}

void GoGitLifecycle::_confirmed() {
	if (activation_offer) {
		if (!_save_setting("enabled", true)) {
			return;
		}
		activation_offer = false;
	}
	if (dont_remind->is_pressed() && !_save_setting(closing ? "remind_on_close" : "remind_on_open", false)) {
		return;
	}
	if (closing) {
		_continue_quit();
	} else {
		_custom_action("git_suite");
	}
}

void GoGitLifecycle::_cancelled() {
	if (owns_job) {
		process.cancel();
		_release();
	}
	if (dont_remind->is_pressed()) {
		_save_setting(closing ? "remind_on_close" : "remind_on_open", false);
	}
	closing = false;
	GoSuiteRegistry::set_quit_pending(false);
	activation_offer = false;
	resume_quit = Callable();
}

void GoGitLifecycle::_custom_action(const String &p_action) {
	if (p_action != "git_suite" && p_action != "pull") {
		return;
	}
	dialog->hide();
	_cancelled();
	if (p_action == "pull" && pull_action.is_valid()) {
		pull_action.call();
	}
	if (GoSuiteRegistry::get_git_dock() != nullptr) {
		EditorDockManager::get_singleton()->focus_dock(GoSuiteRegistry::get_git_dock());
	}
}

bool GoGitLifecycle::request_close(const Callable &p_resume) {
	if (allow_close || !_setting("enabled", false) || !_setting("remind_on_close", true) || DisplayServer::get_singleton()->get_name() == "headless") {
		return false;
	}
	if (!DirAccess::dir_exists_absolute("res://.git") && !FileAccess::exists("res://.git")) {
		return false;
	}
	if (closing || dialog->is_visible()) {
		return true;
	}
	startup_done = true;
	if (owns_job) {
		process.cancel();
		_release();
	}
	closing = true;
	GoSuiteRegistry::set_quit_pending(true);
	resume_quit = p_resume;
	status = go::GitStatus();
	pending_lfs = 0;
	status_failed = false;
	if (GoSuiteRegistry::is_git_job_busy()) {
		status_failed = true;
		_show_result(); // Never start a second operation or wait indefinitely.
		return true;
	}
	dont_remind->set_pressed(false);
	git_suite_button->show();
	pull_button->hide();
	dialog->get_ok_button()->set_disabled(true);
	dialog->get_cancel_button()->set_text(TTR("Cancel"));
	dialog->set_text(TTR("Checking local Git status before closing…"));
	dialog->popup_centered(Size2i(620, 250));
	owns_job = true;
	GoSuiteRegistry::set_git_job_busy(true);
	_launch(Phase::STATUS, go::status(project));
	return true;
}

void GoGitLifecycle::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		GoSuiteRegistry::set_quit_handler(callable_mp(this, &GoGitLifecycle::request_close));
		set_process(true);
	} else if (p_what == NOTIFICATION_PROCESS) {
		_poll();
		if (startup_done || GoSuiteRegistry::is_git_job_busy() || !EditorNode::get_singleton()->is_editor_ready()) {
			return;
		}
		startup_done = true;
		if (DisplayServer::get_singleton()->get_name() == "headless" || !_setting("remind_on_open", true)) {
			return;
		}
		if (!DirAccess::dir_exists_absolute("res://.git") && !FileAccess::exists("res://.git")) {
			return;
		}
		if (!_setting("enabled", false)) {
			EditorSettings *settings = EditorSettings::get_singleton();
			if (bool(settings->get_project_metadata("go_git_suite", "activation_offer_seen", false))) {
				return;
			}
			settings->set_project_metadata("go_git_suite", "activation_offer_seen", true);
			settings->save_project_metadata();
			activation_offer = true;
			dialog->set_text(TTR("This project has a Git repository. Enable the GO. Git Suite? No repository or commit will be created."));
			dialog->get_ok_button()->set_text(TTR("Enable Git Suite"));
			dialog->get_cancel_button()->set_text(TTR("Later"));
			dialog->popup_centered(Size2i(620, 250));
			return;
		}
		owns_job = true;
		GoSuiteRegistry::set_git_job_busy(true);
		_launch(Phase::REMOTE, go::git(project, { "remote", "get-url", "origin" }));
	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		GoSuiteRegistry::set_quit_handler(Callable());
		GoSuiteRegistry::set_quit_pending(false);
		process.cancel();
		_release();
	}
}

GoGitLifecycle::GoGitLifecycle(const Callable &p_pull_action) {
	pull_action = p_pull_action;
	project = go_suite::to_std_string(ProjectSettings::get_singleton()->get_resource_path());
	dialog = memnew(ConfirmationDialog);
	dialog->set_title(TTR("GO. Git Reminder"));
	git_suite_button = dialog->add_button(TTR("Open Git Suite"), true, "git_suite");
	git_suite_button->hide();
	pull_button = dialog->add_button(TTR("Pull"), false, "pull");
	pull_button->hide();
	dialog->connect("confirmed", callable_mp(this, &GoGitLifecycle::_confirmed));
	dialog->connect("canceled", callable_mp(this, &GoGitLifecycle::_cancelled));
	dialog->connect("custom_action", callable_mp(this, &GoGitLifecycle::_custom_action));
	dont_remind = memnew(CheckBox);
	dont_remind->set_text(TTR("Do not show this reminder again for this project"));
	dialog->add_child(dont_remind);
	add_child(dialog);
}
