def can_build(env, platform):
    # GO. Git Suite and Code Agentics are editor-only built-in modules.
    env.module_add_dependencies("go_suite", ["svg"])
    return env.editor_build


def configure(env):
    # A single definition for module and test translation units is essential:
    # upstream TESTS_ENABLED is only set on selected SCons environments.
    if env["tests"]:
        env.Append(CPPDEFINES=["GO_SUITE_TESTS_ENABLED"])
