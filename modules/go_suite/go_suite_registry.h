/**************************************************************************/
/*  go_suite_registry.h                                                   */
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

#include "editor/docks/editor_dock.h"

class GoSuiteRegistry {
	static EditorDock *git_dock;
	static EditorDock *agentics_dock;
	static bool editor_input_locked;
	static bool (*auto_commit_callback)();

public:
	static void set_git_dock(EditorDock *p_dock) { git_dock = p_dock; }
	static EditorDock *get_git_dock() { return git_dock; }
	static void set_agentics_dock(EditorDock *p_dock) { agentics_dock = p_dock; }
	static EditorDock *get_agentics_dock() { return agentics_dock; }

	static void set_editor_input_locked(bool p_locked) { editor_input_locked = p_locked; }
	static bool is_editor_input_locked() { return editor_input_locked; }

	static void set_auto_commit_callback(bool (*p_callback)()) { auto_commit_callback = p_callback; }
	static bool request_auto_commit() { return auto_commit_callback != nullptr && auto_commit_callback(); }
};
