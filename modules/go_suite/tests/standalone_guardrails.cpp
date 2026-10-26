// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT
//
// Standalone test runner for the Godot-independent guardrail logic.
// Compiled by misc/scripts/go_run_guardrails.py on every PR.

#include "go_agentics_path.h"
#include "go_agentics_tools.h"
#include "go_auto_commit.h"
#include "go_commit_message.h"
#include "go_git_lfs.h"
#include "go_git_remote.h"
#include "go_kilo_models.h"
#include "go_suite_core.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace {

int failures = 0;

#define CHECK(condition)                                                                     \
	do {                                                                                     \
		if (!(condition)) {                                                                  \
			std::cerr << "FAIL " << __LINE__ << ": " #condition << std::endl;                \
			failures++;                                                                       \
		}                                                                                    \
	} while (false)

#define CHECK_EQ(actual, expected)                                                           \
	do {                                                                                     \
		const auto _actual = (actual);                                                       \
		const auto _expected = (expected);                                                   \
		if (!(_actual == _expected)) {                                                        \
			std::cerr << "FAIL " << __LINE__ << ": " #actual << " == " #expected            \
					  << " (got '" << _actual << "', expected '" << _expected << "')"        \
					  << std::endl;                                                          \
			failures++;                                                                       \
		}                                                                                    \
	} while (false)

void test_remote_parser() {
	using namespace go;
	const GitRemoteInfo codeberg = parse_remote("https://codeberg.org/knut/foobar.git/");
	CHECK(codeberg.valid);
	CHECK_EQ(host_name(codeberg.host_kind), std::string("codeberg"));
	CHECK_EQ(codeberg.owner, std::string("knut"));
	CHECK_EQ(codeberg.repo, std::string("foobar"));
	CHECK_EQ(codeberg.protocol, std::string("https"));
	CHECK_EQ(codeberg.web_url, std::string("https://codeberg.org/knut/foobar"));

	const GitRemoteInfo github = parse_remote("git@github.com:SampleCraft/GO.-Game-Engine.git");
	CHECK(github.valid);
	CHECK_EQ(host_name(github.host_kind), std::string("github"));
	CHECK_EQ(github.owner, std::string("SampleCraft"));
	CHECK_EQ(github.repo, std::string("GO.-Game-Engine"));
	CHECK_EQ(github.protocol, std::string("ssh"));

	const GitRemoteInfo gitlab = parse_remote("ssh://git@gitlab.com:2222/group/sub/repo.git");
	CHECK(gitlab.valid);
	CHECK_EQ(host_name(gitlab.host_kind), std::string("gitlab"));
	CHECK_EQ(gitlab.owner, std::string("group/sub"));
	CHECK_EQ(gitlab.repo, std::string("repo"));
	CHECK_EQ(gitlab.port, std::string("2222"));
	CHECK_EQ(gitlab.protocol, std::string("ssh"));

	const GitRemoteInfo custom = parse_remote("https://git.example.com:8443/team/project.git");
	CHECK(custom.valid);
	CHECK_EQ(host_name(custom.host_kind), std::string("custom"));
	CHECK_EQ(custom.web_url, std::string("https://git.example.com:8443/team/project"));

	const GitRemoteInfo local = parse_remote("/srv/git/test.git");
	CHECK(local.valid);
	CHECK_EQ(host_name(local.host_kind), std::string("local"));
	CHECK(local.web_url.empty());

	const GitRemoteInfo file_url = parse_remote("file:///srv/git/test.git");
	CHECK(file_url.valid);
	CHECK_EQ(host_name(file_url.host_kind), std::string("local"));

	const GitRemoteInfo credential = parse_remote("https://user:token@github.com/owner/repo.git");
	CHECK(!credential.valid);
	CHECK(!credential.error.empty());
	CHECK(!is_safe_remote_url("https://host/repo.git?token=abcdef"));

	CHECK_EQ(build_remote_url(GitHost::CODEBERG, "knut", "foobar", "https"), std::string("https://codeberg.org/knut/foobar.git"));
	CHECK_EQ(build_remote_url(GitHost::CODEBERG, "knut", "foobar", "ssh"), std::string("git@codeberg.org:knut/foobar.git"));
	CHECK_EQ(build_remote_url(GitHost::GITHUB, "../evil", "repo", "https"), std::string(""));
	CHECK_EQ(host_create_url(GitHost::CODEBERG), std::string("https://codeberg.org/repo/create"));
}

void test_path_guardrails() {
	using namespace go;
	CHECK(is_safe_res_path("res://scripts/player.gd", PathOperation::WRITE));
	CHECK(is_safe_res_path("res://", PathOperation::READ));
	CHECK(!is_safe_res_path("res://../secret.txt", PathOperation::READ));
	CHECK(!is_safe_res_path("res://scripts/../player.gd", PathOperation::WRITE));
	CHECK(!is_safe_res_path("/etc/passwd", PathOperation::READ));
	CHECK(!is_safe_res_path("C:\\Windows\\System32\\config", PathOperation::READ));
	CHECK(!is_safe_res_path("user://secret.txt", PathOperation::READ));
	CHECK(!is_safe_res_path("res://.git/config", PathOperation::WRITE));
	CHECK(!is_safe_res_path("res://.godot/imported/x", PathOperation::READ));
	CHECK(!is_safe_res_path("res://addons/plugin/.git/config", PathOperation::WRITE));

	const SymlinkMap unsafe_symlinks = { { "assets/link.png", "/tmp/outside.png" } };
	CHECK(!is_safe_res_path("res://assets/link.png", PathOperation::READ, unsafe_symlinks));
	const SymlinkMap safe_symlinks = { { "assets/link.png", "res://textures/real.png" } };
	CHECK(is_safe_res_path("res://assets/link.png", PathOperation::READ, safe_symlinks));

	PathValidationResult result = validate_res_path("res:// scenes/hero.tscn ", PathOperation::READ);
	CHECK(result.ok);
	CHECK_EQ(result.relative, std::string(" scenes/hero.tscn"));
	CHECK_EQ(result.normalized, std::string("res:// scenes/hero.tscn"));
}

void test_agentics_tools() {
	using namespace go;
	std::string parse_error;
	const JsonValue definitions = parse_json(GoAgenticsTools::tool_definitions_json(), parse_error);
	CHECK(parse_error.empty());
	CHECK(definitions.type == JsonValue::Type::ARRAY);
	CHECK_EQ(definitions.array_value.size(), GoAgenticsTools::allowed_tool_names().size());
	CHECK(GoAgenticsTools::is_tool_allowed("read_file"));
	CHECK(GoAgenticsTools::is_tool_allowed("auto_commit"));
	CHECK(!GoAgenticsTools::is_tool_allowed("run_shell"));
	CHECK(!GoAgenticsTools::is_tool_allowed("set_api_key"));
	CHECK(contains(GoAgenticsTools::validate_tool_path("res://player.gd", false), "\"ok\":true"));
	CHECK(contains(GoAgenticsTools::validate_tool_path("res://../secret", true), "\"ok\":false"));
	CHECK(contains(GoAgenticsTools::execute_tool("write_file", "{\"path\":\"res://.git/config\"}"), "\"ok\":false"));
}

void test_lfs_rules() {
	using namespace go;
	CHECK(is_text_format("res://scene.tscn"));
	CHECK(is_text_format("res://script.gd"));
	CHECK(is_text_format("res://model.gltf"));
	CHECK(!is_text_format("res://hero.png"));
	CHECK(is_lfs_pattern("res://hero.png"));
	CHECK(is_lfs_pattern("res://ARCHIVE.ZIP"));
	CHECK(is_lfs_pattern("res://model.tres.bin"));
	CHECK(!is_lfs_pattern("res://scene.tscn"));
	CHECK_EQ(suggested_pattern("res://textures/hero.webp"), std::string("*.webp"));

	const std::string existing = "* text=auto eol=lf\n*.gd text eol=lf\n";
	const std::string merged = merge_gitattributes(existing);
	CHECK(go::contains(merged, "* text=auto eol=lf"));
	CHECK(go::contains(merged, "*.gd text eol=lf"));
	CHECK(go::contains(merged, gitattributes_begin_marker()));
	CHECK(go::contains(merged, gitattributes_end_marker()));
	CHECK(go::contains(merged, "*.png filter=lfs diff=lfs merge=lfs -text"));
	const std::string merged_twice = merge_gitattributes(merged);
	CHECK_EQ(std::count(merged_twice.begin(), merged_twice.end(), '*') > 0, true);
	size_t first = merged_twice.find(gitattributes_begin_marker());
	CHECK(first != std::string::npos);
	CHECK(merged_twice.find(gitattributes_begin_marker(), first + 1) == std::string::npos);

	CHECK_EQ(large_file_action_name(classify_large_file("res://new.unknown", 1024, 5, GitHost::CODEBERG).action), std::string("track-lfs"));
	CHECK_EQ(large_file_action_name(classify_large_file("res://big.unknown", 60 * 1024 * 1024, 5, GitHost::CUSTOM).action), std::string("block"));
	LargeFileDecision github = classify_large_file("res://huge.png", 120 * 1024 * 1024, 5, GitHost::GITHUB);
	CHECK_EQ(large_file_action_name(github.action), std::string("warn-quota")); // covered by *.png
	LargeFileDecision github_uncovered = classify_large_file("res://huge.unknown", 120 * 1024 * 1024, 5, GitHost::GITHUB);
	CHECK(github_uncovered.hard_limit);
	CHECK_EQ(large_file_action_name(github_uncovered.action), std::string("block"));
	CHECK_EQ(large_file_action_name(classify_large_file("res://big.gd", 10 * 1024 * 1024, 5, GitHost::NONE).action), std::string("warn-text"));
	CHECK_EQ(large_file_action_name(classify_large_file("res://small.gd", 1024, 5, GitHost::NONE).action), std::string("none"));
	CHECK(is_lfs_pointer_prefix("version https://git-lfs.github.com/spec/v1\noid sha256:..."));
	CHECK(!is_lfs_pointer_prefix("binary data"));
	CHECK(!quota_warning(GitHost::CODEBERG, 1600ULL * 1024 * 1024).empty());
}

void test_kilo_models() {
	using namespace go;
	const std::string json = R"JSON({
		"data": [
			{"id":"kilo-auto/free","name":"Kilo Auto","isFree":true,"supported_parameters":["tools"]},
			{"id":"zero/model","name":"Zero","pricing":{"prompt":"0","completion":"0.0"},"supported_parameters":["tools"]},
			{"id":"paid/model","name":"Paid","pricing":{"prompt":"0.001","completion":"0.002"}},
			{"id":"flag-free/model","name":"Flag","isFree":true}
		]
	})JSON";
	std::string error;
	const std::vector<GoKiloModel> models = parse_models_response(json, error);
	CHECK(error.empty());
	CHECK_EQ(models.size(), size_t(4));
	const std::vector<GoKiloModel> picker = free_models_for_picker(models);
	CHECK_EQ(picker.size(), size_t(3));
	CHECK_EQ(picker.front().id, std::string(DEFAULT_FREE_MODEL));
	bool found_paid = false;
	bool found_zero = false;
	for (const GoKiloModel &model : picker) {
		found_paid = found_paid || model.id == "paid/model";
		found_zero = found_zero || model.id == "zero/model";
	}
	CHECK(found_zero);
	CHECK(!found_paid);
	CHECK(!parse_http_error(429, { "Retry-After: 17" }, "{\"error\":{\"message\":\"slow down\"}}").message.empty());
	const KiloHttpError rate = parse_http_error(429, { "Retry-After: 17" }, "{\"error\":{\"message\":\"slow down\"}}");
	CHECK(rate.is_rate_limit);
	CHECK_EQ(rate.retry_after_seconds, 17);
	CHECK_EQ(rate.message, std::string("slow down"));
	CHECK_EQ(models_endpoint("https://example.test/api/v1/"), std::string("https://example.test/api/v1/models"));
	std::string invalid_error;
	parse_json("{not json", invalid_error);
	CHECK(!invalid_error.empty());
}

void test_commit_messages() {
	using namespace go;
	CHECK_EQ(fallback_commit_message(), std::string("chore: update project files"));
	CHECK(looks_like_conventional_commit("feat: add player"));
	CHECK(!looks_like_conventional_commit("add player"));
	const std::string raw = "```text\nfeat: add a new player controller\n\n- handles input\n- token=should-not-appear\n```";
	const std::string sanitized = sanitize_commit_message(raw);
	CHECK(starts_with(sanitized, "feat:"));
	CHECK(!contains(sanitized, "```"));
	CHECK(!contains(sanitized, "token="));
	CHECK_EQ(sanitize_commit_message(""), fallback_commit_message());
	CHECK(looks_like_secret_file("res://.env"));
	CHECK(looks_like_secret_file("res://config/api_token.json"));
	CHECK(!looks_like_secret_file("res://scripts/player.gd"));
	const std::string diff = "diff --git a/.env b/.env\n+++ b/.env\n+API_KEY=secret\n diff --git a/player.gd b/player.gd\n+print('hello')\n";
	const std::string truncated = truncate_diff(diff, 10000, { ".env", "player.gd" });
	CHECK(contains(truncated, "[GO. redacted sensitive file: .env]"));
	CHECK(!contains(truncated, "API_KEY=secret"));
	CHECK(contains(truncated, "print('hello')"));
	CHECK(contains(build_commit_prompt("M player.gd", "1 file changed", diff), "Conventional Commits"));
}

void test_auto_commit_state() {
	using namespace go;
	AutoCommitPreflight no_changes;
	no_changes.is_repo = true;
	no_changes.has_changes = false;
	no_changes.user_name_set = true;
	no_changes.user_email_set = true;
	AutoCommitStateMachine no_op(make_auto_commit_plan(true, true, false));
	CHECK(!no_op.begin(no_changes));
	CHECK(!no_op.is_locked());
	CHECK(!no_op.is_active());
	CHECK(no_op.invariant_holds());
	CHECK_EQ(stage_name(no_op.current_stage()), std::string("failed"));

	AutoCommitPreflight good;
	good.is_repo = true;
	good.has_changes = true;
	good.user_name_set = true;
	good.user_email_set = true;
	AutoCommitPlan plan = make_auto_commit_plan(false, false, true);
	bool has_push = false;
	for (AutoCommitStage stage : plan.stages) {
		has_push = has_push || stage == AutoCommitStage::PUSH;
	}
	CHECK(!has_push);
	AutoCommitStateMachine success(plan);
	CHECK(success.begin(good));
	CHECK(success.is_locked());
	CHECK(success.is_active());
	CHECK(success.invariant_holds());
	for (size_t i = 1; i < plan.stages.size(); ++i) {
		CHECK(success.advance());
		CHECK(success.invariant_holds());
	}
	CHECK(success.finish());
	CHECK(!success.is_locked());
	CHECK(success.invariant_holds());
	CHECK_EQ(stage_name(success.current_stage()), std::string("done"));

	AutoCommitStateMachine failure(plan);
	CHECK(failure.begin(good));
	CHECK(failure.advance());
	CHECK(!failure.fail("network error"));
	CHECK(!failure.is_locked());
	CHECK(!failure.is_active());
	CHECK(failure.invariant_holds());
	CHECK_EQ(failure.error(), std::string("network error"));
	CHECK_EQ(stage_name(failure.current_stage()), std::string("failed"));

	AutoCommitPlan push_plan = make_auto_commit_plan(true, false, false);
	CHECK(push_plan.will_push);
	CHECK_EQ(stage_name(push_plan.stages.back()), std::string("push"));
}

} // namespace

int main() {
	test_remote_parser();
	test_path_guardrails();
	test_agentics_tools();
	test_lfs_rules();
	test_kilo_models();
	test_commit_messages();
	test_auto_commit_state();
	if (failures > 0) {
		std::cerr << failures << " guardrail check(s) failed." << std::endl;
		return 1;
	}
	std::cout << "OK: GO. standalone guardrails passed" << std::endl;
	return 0;
}
