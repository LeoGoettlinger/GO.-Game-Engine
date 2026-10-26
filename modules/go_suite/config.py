def can_build(env, platform):
    # GO. Git Suite and Code Agentics are editor-only built-in modules.
    env.module_add_dependencies("go_suite", ["svg"])
    return env.editor_build


def configure(env):
    pass
