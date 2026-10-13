#pragma once

#include <raylib.h>

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace typist {

// Slug GPU outline renderer for exercise and interface text. Unsupported GL
// contexts fall back to the supplied raylib font.
class SlugText {
public:
    SlugText(const std::vector<std::string>& lessons, Font fallback);
    ~SlugText();
    SlugText(const SlugText&) = delete;
    SlugText& operator=(const SlugText&) = delete;
    void Shutdown();
    void EnsureGlyphs(std::string_view text);

    float Measure(char32_t codepoint, float size, Font fallback = {}) const;
    float Measure(std::span<const char32_t> text, float size, float spacing = 1,
                  Font fallback = {}) const;
    float Measure(std::string_view text, float size, float spacing = 1,
                  Font fallback = {}) const;
    void Draw(char32_t codepoint, Vector2 position, float size, Color tint,
              Font fallback = {}) const;
    void Draw(std::string_view text, Vector2 position, float size, Color tint,
              float spacing = 1, Font fallback = {}) const;
    bool Active() const;

private:
    struct Impl;
    Impl* impl_;
};

}  // namespace typist
