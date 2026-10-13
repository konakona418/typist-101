#pragma once

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace typist {

struct LayoutPoint { float x = 0; float y = 0; };
struct LayoutLine { std::size_t begin = 0; std::size_t end = 0; float y = 0; };

class TextLayout {
public:
    using Measure = std::function<float(char32_t)>;
    void Build(const std::vector<char32_t>& text, float width, float line_height,
               const Measure& measure, float newline_hint_width = 0);
    LayoutPoint CursorPosition(std::size_t index) const;
    std::size_t LineFor(std::size_t index) const;
    std::pair<std::size_t, std::size_t> VisibleLines(float scroll, float height) const;
    float CenteredScroll(std::size_t line, float viewport_height) const;
    const std::vector<LayoutLine>& Lines() const { return lines_; }
    std::size_t Size() const { return points_.empty() ? 0 : points_.size() - 1; }
    float Height() const { return lines_.size() * line_height_; }
    float LineHeight() const { return line_height_; }

private:
    std::vector<LayoutPoint> points_;
    std::vector<std::size_t> line_for_;
    std::vector<LayoutLine> lines_;
    float line_height_ = 1;
};

}  // namespace typist
