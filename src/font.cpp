#include "font.h"
#include "utf8.h"

#include <fontconfig/fontconfig.h>

#include <set>

namespace typist {

namespace {
std::string FindFontFile() {
    if (!FcInit()) return {};
    FcPattern* pattern = FcPatternCreate();
    // raylib's stb_truetype path cannot load TrueType Collections (.ttc).
    // Prefer an individual CJK TrueType face for the non-Slug fallback.
    FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8*>("IBM Plex Sans SC"));
    FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult match_result = FcResultNoMatch;
    FcPattern* match = FcFontMatch(nullptr, pattern, &match_result);
    std::string path;
    if (match) {
        FcChar8* file = nullptr;
        if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file)
            path = reinterpret_cast<const char*>(file);
        FcPatternDestroy(match);
    }
    FcPatternDestroy(pattern);
    return path;
}
}  // namespace

Font LoadExerciseFont(const std::vector<std::string>& lessons, int pixel_size) {
    std::set<int> characters;
    for (int c = 32; c < 127; ++c) characters.insert(c);
    for (const auto& lesson : lessons)
        for (char32_t c : DecodeUtf8(lesson))
            if (c >= U' ' && c != 0x7f) characters.insert(static_cast<int>(c));
    std::vector<int> codepoints(characters.begin(), characters.end());
    const auto path = FindFontFile();
    if (path.empty()) return GetFontDefault();
    Font font = LoadFontEx(path.c_str(), pixel_size, codepoints.data(),
                           static_cast<int>(codepoints.size()));
    if (font.texture.id == 0) return GetFontDefault();
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    return font;
}

}  // namespace typist
