#include "slug_text.h"
#include "utf8.h"

#include <fontconfig/fontconfig.h>
#include <epoxy/gl.h>

#include <algorithm>
#include <map>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>

#include "slug.h"

namespace typist {

namespace {

std::pair<std::string, int> ResolveFace(int codepoint) {
    FcPattern* pattern = FcPatternCreate();
    FcCharSet* charset = FcCharSetCreate();
    FcCharSetAddChar(charset, static_cast<FcChar32>(codepoint));
    FcPatternAddCharSet(pattern, FC_CHARSET, charset);
    FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);
    FcPatternAddString(pattern, FC_FAMILY,
                       reinterpret_cast<const FcChar8*>("Noto Sans CJK SC"));
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult result = FcResultNoMatch;
    FcPattern* match = FcFontMatch(nullptr, pattern, &result);
    std::pair<std::string, int> face;
    if (match) {
        FcChar8* file = nullptr;
        if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file) {
            face.first = reinterpret_cast<const char*>(file);
            FcPatternGetInteger(match, FC_INDEX, 0, &face.second);
        }
        FcPatternDestroy(match);
    }
    FcCharSetDestroy(charset);
    FcPatternDestroy(pattern);
    return face;
}

}  // namespace

struct SlugText::Impl {
    struct Face {
        PSlugFont font = nullptr;
        std::string file;
        int index = 0;
        std::set<int> codepoints;
    };
    Font fallback{};
    std::vector<Face> faces;
    std::unordered_map<int, std::size_t> face_for_codepoint;
    std::set<int> attempted;
    bool available = false;
    bool active = false;
};

SlugText::SlugText(const std::vector<std::string>& lessons, Font fallback) : impl_(new Impl) {
    impl_->fallback = fallback;
    int major = 0;
    int minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major < 4 || (major == 4 && minor < 3) || !FcInit()) {
        TraceLog(LOG_WARNING, "Slug requires OpenGL 4.3; using raylib font fallback");
        return;
    }
    impl_->available = true;
    std::string initial;
    for (int c = 32; c < 127; ++c) initial.push_back(static_cast<char>(c));
    for (const auto& lesson : lessons) initial += lesson;
    EnsureGlyphs(initial);
    TraceLog(LOG_INFO, "Slug: %d font face(s), %d codepoint(s)",
             static_cast<int>(impl_->faces.size()),
             static_cast<int>(impl_->face_for_codepoint.size()));
}

void SlugText::EnsureGlyphs(std::string_view text) {
    if (!impl_->available) return;

    using FaceKey = std::pair<std::string, int>;
    std::map<FaceKey, std::set<int>> additions;
    for (char32_t c : DecodeUtf8(text)) {
        if (c < U' ' || c == 0x7f || !impl_->attempted.insert(static_cast<int>(c)).second) continue;
        auto key = ResolveFace(static_cast<int>(c));
        if (!key.first.empty()) additions[key].insert(static_cast<int>(c));
    }
    for (const auto& [key, added] : additions) {
        auto existing = std::find_if(impl_->faces.begin(), impl_->faces.end(), [&](const Impl::Face& face) {
            return face.file == key.first && face.index == key.second;
        });
        std::set<int> combined = added;
        if (existing != impl_->faces.end())
            combined.insert(existing->codepoints.begin(), existing->codepoints.end());
        const std::vector<int> points(combined.begin(), combined.end());
        PSlugFont replacement = LoadFontSlugEx(key.first.c_str(), key.second,
                                               points.data(), static_cast<int>(points.size()));
        if (!replacement) continue;
        std::size_t index;
        if (existing == impl_->faces.end()) {
            index = impl_->faces.size();
            impl_->faces.push_back({replacement, key.first, key.second, std::move(combined)});
        } else {
            index = static_cast<std::size_t>(existing - impl_->faces.begin());
            UnloadFontSlug(existing->font);
            existing->font = replacement;
            existing->codepoints = std::move(combined);
        }
        for (int codepoint : impl_->faces[index].codepoints)
            if (replacement->HasGlyph(codepoint)) impl_->face_for_codepoint[codepoint] = index;
    }
    impl_->active = !impl_->faces.empty();
}

SlugText::~SlugText() {
    Shutdown();
    delete impl_;
}

void SlugText::Shutdown() {
    for (auto& face : impl_->faces) UnloadFontSlug(face.font);
    impl_->faces.clear();
    impl_->face_for_codepoint.clear();
    impl_->attempted.clear();
    impl_->available = false;
    impl_->active = false;
}

float SlugText::Measure(char32_t codepoint, float size, Font fallback) const {
    const auto it = impl_->face_for_codepoint.find(static_cast<int>(codepoint));
    if (it != impl_->face_for_codepoint.end()) {
        PSlugFont font = impl_->faces[it->second].font;
        return font->GetAdvance(static_cast<int>(codepoint), 0)
               * font->GetScaleForPixelHeight(size);
    }
    const int c = static_cast<int>(codepoint);
    return MeasureTextCodepoints(fallback.texture.id ? fallback : impl_->fallback,
                                 &c, 1, size, 0).x;
}

float SlugText::Measure(std::span<const char32_t> text, float size, float spacing,
                        Font fallback) const {
    float width = 0;
    for (char32_t c : text) width += Measure(c, size, fallback) + spacing;
    return text.empty() ? 0 : width - spacing;
}

float SlugText::Measure(std::string_view text, float size, float spacing,
                        Font fallback) const {
    const auto points = DecodeUtf8(text);
    return Measure(std::span<const char32_t>(points), size, spacing, fallback);
}

void SlugText::Draw(char32_t codepoint, Vector2 position, float size, Color tint,
                    Font fallback) const {
    if (codepoint == U' ' || codepoint == U'\n') return;
    const auto it = impl_->face_for_codepoint.find(static_cast<int>(codepoint));
    if (it != impl_->face_for_codepoint.end()) {
        DrawTextCodepointSlug(impl_->faces[it->second].font,
                              static_cast<int>(codepoint), position, size, tint);
        return;
    }
    DrawTextCodepoint(fallback.texture.id ? fallback : impl_->fallback,
                      static_cast<int>(codepoint), position, size, tint);
}

void SlugText::Draw(std::string_view text, Vector2 position, float size, Color tint,
                    float spacing, Font fallback) const {
    const auto points = DecodeUtf8(text);
    for (char32_t c : points) {
        Draw(c, position, size, tint, fallback);
        position.x += Measure(c, size, fallback) + spacing;
    }
}

bool SlugText::Active() const { return impl_->active; }

}  // namespace typist
