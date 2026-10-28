/**************************************************************************/
/*  go_project_setup.cpp                                                  */
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

#include "go_project_setup.h"

#include "go_git_lfs.h"
#include "go_system_git.h"
#include "go_system_git_runner.h"

#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"

#include <string>
#include <vector>

std::vector<std::string> GoProjectSetup::core_plugin_directories() {
	return {
		"aseprite_wizard",
		"beehave",
		"dialogue_manager",
		"godot_state_charts",
		"gut",
		"phantom_camera",
		"proton_scatter",
	};
}

bool GoProjectSetup::apply(const String &p_project_path, bool p_git_suite, bool p_git_lfs, String &r_warning) {
	r_warning = String();
	const String project_godot = p_project_path.path_join("project.godot");
	Ref<ConfigFile> config;
	config.instantiate();
	if (config->load(project_godot) != OK) {
		r_warning = TTR("GO. could not read the new project.godot; Git Suite defaults were not written.");
		return false;
	}

	String warnings;
	String executable_dir = OS::get_singleton() != nullptr ? OS::get_singleton()->get_executable_path().get_base_dir() : String();
	// Development binaries live in bin/; release bundles keep data beside the editor.
	if (!FileAccess::exists(executable_dir.path_join("go_bundled_plugins/plugins.json")) &&
			FileAccess::exists(executable_dir.get_base_dir().path_join("go_bundled_plugins/plugins.json"))) {
		executable_dir = executable_dir.get_base_dir();
	}
	const String splash_source = executable_dir.path_join("go_project_template").path_join("splash.png");
	if (FileAccess::exists(splash_source)) {
		if (DirAccess::copy_absolute(splash_source, p_project_path.path_join("splash.png")) == OK) {
			config->set_value("application", "boot_splash/image", "res://splash.png");
		} else {
			warnings += TTR("GO. boot splash could not be copied.") + "\n";
		}
	} else {
		warnings += TTR("GO. boot splash was not found next to the editor.") + "\n";
	}

	config->set_value("go", "git_suite/enabled", p_git_suite);
	config->set_value("go", "git_suite/remind_on_open", p_git_suite);
	config->set_value("go", "git_suite/remind_on_close", p_git_suite);
	config->set_value("go", "git_suite/lfs/enabled", p_git_suite && p_git_lfs);
	config->set_value("go", "git_suite/lfs/large_file_warning_mib", 5);

	if (p_git_suite) {
		const String bundled_root = executable_dir.path_join("go_bundled_plugins");
		const String addons_target = p_project_path.path_join("addons");
		if (DirAccess::make_dir_recursive_absolute(addons_target) != OK) {
			warnings += TTR("GO. could not create the project addons folder.") + "\n";
		}
		int copied = 0;
		for (const std::string &directory : core_plugin_directories()) {
			const String source = bundled_root.path_join(String::utf8(directory.c_str())).path_join("addons");
			if (!DirAccess::dir_exists_absolute(source)) {
				continue;
			}
			Ref<DirAccess> access = DirAccess::create_for_path(source);
			if (access.is_valid() && access->copy_dir(source, addons_target) == OK) {
				copied++;
			}
		}
		config->set_value("go", "bundled_plugins/copied", copied > 0);
		if (copied == 0) {
			warnings += TTR("No bundled plugins were found next to the editor; the project is still usable.") + "\n";
		}

		if (!DirAccess::dir_exists_absolute(p_project_path.path_join(".git")) && !FileAccess::exists(p_project_path.path_join(".git"))) {
			go_suite::GoSystemGitRunner runner(p_project_path);
			std::string output;
			int exit_code = -1;
			const go::GitCommand init = go::git(go_suite::to_std_string(p_project_path), { "init", "-b", "main" });
			if (!runner.run(init, output, exit_code) || exit_code != 0) {
				warnings += TTR("Project was created, but git init failed. Use the Git Suite to initialise it later.") + "\n";
			}
		}

		if (p_git_lfs) {
			// Rules must exist even when git-lfs is not installed yet.
			const String attributes_path = p_project_path.path_join(".gitattributes");
			std::string existing;
			if (FileAccess::exists(attributes_path)) {
				Ref<FileAccess> input = FileAccess::open(attributes_path, FileAccess::READ);
				if (input.is_valid()) {
					existing = go_suite::to_std_string(input->get_as_text());
				}
			}
			Ref<FileAccess> attributes = FileAccess::open(attributes_path, FileAccess::WRITE);
			if (attributes.is_valid()) {
				attributes->store_string(go_suite::to_godot_string(go::merge_gitattributes(existing)));
				attributes->close();
			}
			go_suite::GoSystemGitRunner runner(p_project_path);
			std::string output;
			int exit_code = -1;
			const go::GitCommand install = go::lfs_install_local(go_suite::to_std_string(p_project_path));
			const go::GitCommand track = go::lfs_track_many(go_suite::to_std_string(p_project_path), go::default_lfs_patterns());
			if (!runner.run(install, output, exit_code) || exit_code != 0 || !runner.run(track, output, exit_code) || exit_code != 0) {
				warnings += TTR("Git LFS setup did not complete. Large assets may need manual tracking.") + "\n";
			}
		}
	}

	if (config->save(project_godot) != OK) {
		r_warning = TTR("GO. could not save the new project.godot settings.");
		return false;
	}
	r_warning = warnings.strip_edges();
	return true;
}
