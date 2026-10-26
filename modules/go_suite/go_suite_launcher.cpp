// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_suite_launcher.h"

#include "go_suite_registry.h"

#include "core/object/callable_mp.h"
#include "core/string/ustring.h"
#include "editor/docks/editor_dock.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"

void GoSuiteLauncher::_on_git_toggled(bool p_pressed) {
	EditorDock *dock = GoSuiteRegistry::get_git_dock();
	if (dock == nullptr) {
		return;
	}
	if (p_pressed) {
		dock->make_visible();
	} else {
		dock->close();
	}
}

void GoSuiteLauncher::_on_agentics_toggled(bool p_pressed) {
	EditorDock *dock = GoSuiteRegistry::get_agentics_dock();
	if (dock == nullptr) {
		return;
	}
	if (p_pressed) {
		dock->make_visible();
	} else {
		dock->close();
	}
}

void GoSuiteLauncher::_bind_methods() {
}

GoSuiteLauncher::GoSuiteLauncher() {
	set_custom_minimum_size(Size2(180, 0) * EDSCALE);

	Label *title = memnew(Label);
	title->set_text(TTR("GO. Suites"));
	title->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	add_child(title);

	git_button = memnew(Button);
	git_button->set_text(TTR("Git Suite"));
	git_button->set_toggle_mode(true);
	git_button->set_pressed_no_signal(true);
	git_button->set_tooltip_text(TTR("Show or hide the GO. Git Suite dock."));
	git_button->connect("toggled", callable_mp(this, &GoSuiteLauncher::_on_git_toggled));
	add_child(git_button);

	agentics_button = memnew(Button);
	agentics_button->set_text(TTR("Code Agentics"));
	agentics_button->set_toggle_mode(true);
	agentics_button->set_pressed_no_signal(true);
	agentics_button->set_tooltip_text(TTR("Show or hide the Code Agentics dock."));
	agentics_button->connect("toggled", callable_mp(this, &GoSuiteLauncher::_on_agentics_toggled));
	add_child(agentics_button);
}
