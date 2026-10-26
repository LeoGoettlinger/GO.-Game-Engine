// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_suite_icons.h"

#include "go_suite_icons.gen.h"

#include "core/io/image.h"
#include "core/math/color.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "modules/svg/image_loader_svg.h"
#include "scene/resources/image_texture.h"

namespace go_suite {

Ref<Texture2D> make_dock_icon(const char *p_svg) {
	Ref<Image> image;
	image.instantiate();
	HashMap<Color, Color> color_map;
	const Error err = ImageLoaderSVG::create_image_from_string(image, String::utf8(p_svg), 1.0f, false, color_map);
	if (err == OK && image.is_valid() && !image->is_empty()) {
		return ImageTexture::create_from_image(image);
	}

	// Deterministic fallback if the SVG module is unavailable at runtime.
	Ref<Image> fallback = Image::create_empty(16, 16, false, Image::FORMAT_RGBA8);
	fallback->fill(Color(0.33f, 0.80f, 0.95f, 1.0f));
	return ImageTexture::create_from_image(fallback);
}

Ref<Texture2D> git_dock_icon() {
	return make_dock_icon(GO_SUITE_GIT_DOCK_SVG);
}

Ref<Texture2D> agentics_dock_icon() {
	return make_dock_icon(GO_SUITE_AGENTICS_DOCK_SVG);
}

Ref<Texture2D> launcher_icon() {
	return make_dock_icon(GO_SUITE_LAUNCHER_SVG);
}

} // namespace go_suite
