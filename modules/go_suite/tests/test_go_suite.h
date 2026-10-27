// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../go_agentics_path.h"
#include "../go_agentics_tools.h"
#include "../go_auto_commit.h"
#include "../go_commit_message.h"
#include "../go_git_lfs.h"
#include "../go_git_remote.h"
#include "../go_kilo_models.h"
#include "../go_suite_core.h"
#include "../go_system_git.h"

#include "tests/test_macros.h"

#include <string>

namespace TestGoSuite {

TEST_CASE("[GO.][GitRemote] Known hosts, SSH, custom and local remotes") {
	const go::GitRemoteInfo codeberg = go::parse_remote("https://codeberg.org/knut/foobar.git/");
	CHECK(codeberg.valid);
	CHECK(codeberg.owner == "knut");
	CHECK(codeberg.repo == "foobar");
	CHECK(go::host_name(codeberg.host_kind) == "codeberg");
	CHECK(codeberg.web_url == "https://codeberg.org/knut/foobar");

	const go::GitRemoteInfo github = go::parse_remote("git@github.com:SampleCraft/GO.-Game-Engine.git");
	CHECK(github.valid);
	CHECK(github.owner == "SampleCraft");
	CHECK(github.repo == "GO.-Game-Engine");
	CHECK(github.protocol == "ssh");

	const go::GitRemoteInfo gitlab = go::parse_remote("ssh://git@gitlab.com:2222/group/sub/repo.git");
	CHECK(gitlab.valid);
	CHECK(gitlab.owner == "group/sub");
	CHECK(gitlab.port == "2222");

	const go::GitRemoteInfo local = go::parse_remote("/srv/git/test.git");
	CHECK(local.valid);
	CHECK(go::host_name(local.host_kind) == "local");
	CHECK(local.web_url.empty());

	const go::GitRemoteInfo credential = go::parse_remote("https://user:token@github.com/owner/repo.git");
	CHECK_FALSE(credential.valid);
	CHECK_FALSE(go::is_safe_remote_url("https://host/repo.git?token=abcdef"));

	CHECK(go::build_remote_url(go::GitHost::CODEBERG, "knut", "foobar", "https") == "https://codeberg.org/knut/foobar.git");
	CHECK(go::build_remote_url(go::GitHost::CODEBERG, "knut", "foobar", "ssh") == "git@codeberg.org:knut/foobar.git");
	CHECK(go::build_remote_url(go::GitHost::GITHUB, "../evil", "repo", "https").empty());
}

TEST_CASE("[GO.][PathGuardrails] Only safe res:// project paths") {
	CHECK(go::is_safe_res_path("res://scripts/player.gd", go::PathOperation::WRITE));
	CHECK_FALSE(go::is_safe_res_path("res://../secret.txt", go::PathOperation::READ));
	CHECK_FALSE(go::is_safe_res_path("res://scripts/../player.gd", go::PathOperation::WRITE));
	CHECK_FALSE(go::is_safe_res_path("/etc/passwd", go::PathOperation::READ));
	CHECK_FALSE(go::is_safe_res_path("C:\\Windows\\System32", go::PathOperation::READ));
	CHECK_FALSE(go::is_safe_res_path("res://.git/config", go::PathOperation::WRITE));
	CHECK_FALSE(go::is_safe_res_path("res://.godot/imported/x", go::PathOperation::READ));

	const go::SymlinkMap unsafe_links = { { "assets/link.png", "/tmp/outside.png" } };
	CHECK_FALSE(go::is_safe_res_path("res://assets/link.png", go::PathOperation::READ, unsafe_links));
	const go::SymlinkMap safe_links = { { "assets/link.png", "res://textures/real.png" } };
	CHECK(go::is_safe_res_path("res://assets/link.png", go::PathOperation::READ, safe_links));
}

TEST_CASE("[GO.][AgenticsTools] Whitelist, schema and path guard") {
	std::string error;
	const go::JsonValue definitions = go::parse_json(go::GoAgenticsTools::tool_definitions_json(), error);
	CHECK(error.empty());
	CHECK(static_cast<int>(definitions.type) == static_cast<int>(go::JsonValue::Type::ARRAY));
	CHECK(go::GoAgenticsTools::is_tool_allowed("read_file"));
	CHECK_FALSE(go::GoAgenticsTools::is_tool_allowed("run_shell"));
	CHECK(go::contains(go::GoAgenticsTools::validate_tool_path("res://player.gd", false), "\"ok\":true"));
	CHECK(go::contains(go::GoAgenticsTools::validate_tool_path("res://../secret", true), "\"ok\":false"));
}

TEST_CASE("[GO.][LFS] Text formats, managed block and large files") {
	CHECK(go::is_text_format("res://scene.tscn"));
	CHECK_FALSE(go::is_text_format("res://hero.png"));
	CHECK(go::is_lfs_pattern("res://hero.png"));
	CHECK_FALSE(go::is_lfs_pattern("res://scene.tscn"));

	const std::string merged = go::merge_gitattributes("* text=auto eol=lf\n*.gd text eol=lf\n");
	CHECK(go::contains(merged, "* text=auto eol=lf"));
	CHECK(go::contains(merged, "*.gd text eol=lf"));
	CHECK(go::contains(merged, go::gitattributes_begin_marker()));
	CHECK(go::contains(merged, go::gitattributes_end_marker()));
	CHECK(go::contains(go::merge_gitattributes(merged), "*.png filter=lfs diff=lfs merge=lfs -text"));

	CHECK(go::large_file_action_name(go::classify_large_file("res://new.unknown", 1024, 5, go::GitHost::CODEBERG).action) == "track-lfs");
	CHECK(go::large_file_action_name(go::classify_large_file("res://big.unknown", 60 * 1024 * 1024, 5, go::GitHost::CUSTOM).action) == "block");
	CHECK(go::classify_large_file("res://huge.unknown", 120 * 1024 * 1024, 5, go::GitHost::GITHUB).hard_limit);
	CHECK(go::is_lfs_pointer_prefix("version https://git-lfs.github.com/spec/v1\n"));
}

TEST_CASE("[GO.][Kilo] Free-model filter and 429 parsing") {
	const std::string json = R"JSON({"data":[
		{"id":"kilo-auto/free","name":"Kilo Auto","isFree":true,"supported_parameters":["tools"]},
		{"id":"zero/model","name":"Zero","pricing":{"prompt":"0","completion":"0.0"}},
		{"id":"paid/model","name":"Paid","pricing":{"prompt":"0.001","completion":"0.002"}}
	]})JSON";
	std::string error;
	const std::vector<go::GoKiloModel> models = go::parse_models_response(json, error);
	CHECK(error.empty());
	CHECK(models.size() == 3);
	const std::vector<go::GoKiloModel> picker = go::free_models_for_picker(models);
	CHECK(picker.size() == 2);
	CHECK(picker.front().id == go::DEFAULT_FREE_MODEL);
	bool paid_found = false;
	for (const go::GoKiloModel &model : picker) {
		paid_found = paid_found || model.id == "paid/model";
	}
	CHECK_FALSE(paid_found);
	const go::KiloHttpError rate = go::parse_http_error(429, { "Retry-After: 17" }, "{\"error\":{\"message\":\"slow down\"}}");
	CHECK(rate.is_rate_limit);
	CHECK(rate.retry_after_seconds == 17);
	CHECK(rate.message == "slow down");
}

TEST_CASE("[GO.][AutoCommit] No-op, success and failure always unlock") {
	go::AutoCommitPreflight no_changes;
	no_changes.is_repo = true;
	no_changes.has_changes = false;
	no_changes.user_name_set = true;
	no_changes.user_email_set = true;
	go::AutoCommitStateMachine no_op(go::make_auto_commit_plan(true, true, false));
	CHECK_FALSE(no_op.begin(no_changes));
	CHECK_FALSE(no_op.is_locked());
	CHECK(no_op.invariant_holds());

	go::AutoCommitPreflight good;
	good.is_repo = true;
	good.has_changes = true;
	good.user_name_set = true;
	good.user_email_set = true;
	go::AutoCommitPlan plan = go::make_auto_commit_plan(false, false, true);
	go::AutoCommitStateMachine success(plan);
	CHECK(success.begin(good));
	CHECK(success.is_locked());
	for (size_t i = 1; i < plan.stages.size(); ++i) {
		CHECK(success.advance());
	}
	CHECK(success.finish());
	CHECK_FALSE(success.is_locked());
	CHECK(success.invariant_holds());

	go::AutoCommitStateMachine failure(plan);
	CHECK(failure.begin(good));
	CHECK_FALSE(failure.fail("network error"));
	CHECK_FALSE(failure.is_locked());
	CHECK(failure.invariant_holds());
	CHECK(failure.error() == "network error");
}

TEST_CASE("[GO.][CommitMessage] Sanitising and secret-aware diff truncation") {
	CHECK(go::sanitize_commit_message("").empty() == false);
	const std::string sanitized = go::sanitize_commit_message("```\nfeat: add player\n\n- handles input\n```");
	CHECK(go::starts_with(sanitized, "feat:"));
	CHECK_FALSE(go::contains(sanitized, "```"));
	CHECK(go::looks_like_secret_file("res://.env"));
	CHECK_FALSE(go::looks_like_secret_file("res://player.gd"));
	const std::string diff = "diff --git a/.env b/.env\n+API_KEY=secret\n diff --git a/player.gd b/player.gd\n+print('hello')\n";
	const std::string truncated = go::truncate_diff(diff, 10000, { ".env", "player.gd" });
	CHECK(go::contains(truncated, "[GO. redacted sensitive file: .env]"));
	CHECK_FALSE(go::contains(truncated, "API_KEY=secret"));
	CHECK(go::contains(truncated, "print('hello')"));
}

TEST_CASE("[GO.][SystemGit] Safe argv commands and Git/LFS output parsers") {
	const go::GitCommand status_cmd = go::status("/tmp/project");
	CHECK(status_cmd.valid);
	CHECK(status_cmd.executable == "git");
	CHECK(status_cmd.arguments[0] == "-C");
	CHECK(status_cmd.arguments[1] == "/tmp/project");
	CHECK(status_cmd.arguments[2] == "status");
	CHECK_FALSE(go::status("-unsafe").valid);

	const go::GitCommand remote_cmd = go::remote_add("/tmp/project", "origin", "https://codeberg.org/owner/repo.git");
	CHECK(remote_cmd.valid);
	CHECK(remote_cmd.arguments.back() == "https://codeberg.org/owner/repo.git");
	CHECK_FALSE(go::remote_add("/tmp/project", "origin", "https://user:secret@example.com/repo.git").valid);

	const go::GitCommand push_cmd = go::push("/tmp/project", "origin", "main", true);
	CHECK(push_cmd.valid);
	CHECK(push_cmd.arguments[3] == "-u");

	const std::string porcelain =
			"## main...origin/main [ahead 1, behind 2]\n"
			"M  file.gd\n"
			"?? new.gd\n"
			"UU conflict.txt\n"
			"R  old.txt -> new.txt\n";
	const go::GitStatus parsed = go::parse_status_porcelain(porcelain);
	CHECK(parsed.branch == "main");
	CHECK(parsed.ahead == 1);
	CHECK(parsed.behind == 2);
	CHECK(parsed.has_changes);
	CHECK(parsed.merge_conflict);
	CHECK(parsed.entries.size() == 4);

	int ahead = 0;
	int behind = 0;
	CHECK(go::parse_ahead_behind("3\t4", ahead, behind));
	CHECK(ahead == 3);
	CHECK(behind == 4);

	const std::vector<go::GitLogEntry> entries = go::parse_log(
			"abc123\x1f" "Author\x1f" "2026-10-27\x1f" "initial commit\x1e");
	CHECK(entries.size() == 1);
	CHECK(entries.front().hash == "abc123");
	CHECK(entries.front().subject == "initial commit");

	const go::LfsSnapshot snapshot = go::parse_lfs_ls_files(
			"oid123 * hero.png (2.0 MB)\n"
			"oid456 - music.ogg (512 KB)\n");
	CHECK(snapshot.files.size() == 2);
	CHECK(snapshot.files[0].materialized);
	CHECK_FALSE(snapshot.files[1].materialized);
	CHECK(snapshot.total_bytes >= 2ULL * 1024 * 1024);

	CHECK(go::is_lfs_pointer_content("version https://git-lfs.github.com/spec/v1\noid sha256:abc\nsize 123\n"));
	CHECK_FALSE(go::is_lfs_pointer_content("not a pointer"));

	const std::vector<std::pair<std::string, uint64_t>> files = {
		{ "res://hero.unknown", 6ULL * 1024 * 1024 },
	};
	const std::vector<std::string> patterns = go::lfs_patterns_for_files(files, 5, go::GitHost::CODEBERG);
	CHECK(patterns.empty() == false);
	CHECK(patterns.front() == "*.unknown");
}

} // namespace TestGoSuite
