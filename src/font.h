#pragma once

#include <raylib.h>

#include <string>
#include <vector>

namespace typist {

// Fallback atlas for systems without Slug support or a matching Slug glyph.
Font LoadExerciseFont(const std::vector<std::string>& lessons, int pixel_size);

}  // namespace typist
