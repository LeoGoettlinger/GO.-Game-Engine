// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "scene/resources/texture.h"

namespace go_suite {

Ref<Texture2D> make_dock_icon(const char *p_svg);
Ref<Texture2D> git_dock_icon();
Ref<Texture2D> agentics_dock_icon();
Ref<Texture2D> launcher_icon();

} // namespace go_suite
