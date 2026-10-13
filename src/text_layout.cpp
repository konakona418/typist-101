#include "text_layout.h"

#include <algorithm>
#include <cmath>

namespace typist {

namespace {
bool IsWord(char32_t c) {
    return c >= 0x21 && c <= 0x7e;
}
}

void TextLayout::Build(const std::vector<char32_t>& text, float width, float line_height,
                       const Measure& measure, float newline_hint_width) {
    width = std::max(1.0f, width);
    line_height_ = std::max(1.0f, line_height);
    points_.assign(text.size() + 1, {});
    line_for_.assign(text.size() + 1, 0);
    lines_.clear();
    std::size_t begin = 0;
    float x = 0;
    float y = 0;
    auto finish_line = [&](std::size_t end) {
        lines_.push_back({begin, end, y});
        begin = end;
        x = 0;
        y += line_height_;
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == U'\n') {
            if (x > 0 && x + newline_hint_width > width) finish_line(i);
            points_[i] = {x, y};
            line_for_[i] = lines_.size();
            finish_line(i);
            begin = i + 1;
            continue;
        }
        if (x > 0 && IsWord(text[i]) && (i == 0 || !IsWord(text[i - 1]))) {
            float word_width = 0;
            for (std::size_t j = i; j < text.size() && IsWord(text[j]); ++j)
                word_width += std::max(0.0f, measure(text[j]));
            if (word_width <= width && x + word_width > width) finish_line(i);
        }
        const float advance = std::max(0.0f, measure(text[i]));
        if (x > 0 && x + advance > width) finish_line(i);
        points_[i] = {x, y};
        line_for_[i] = lines_.size();
        x += advance;
    }
    points_[text.size()] = {x, y};
    line_for_[text.size()] = lines_.size();
    lines_.push_back({begin, text.size(), y});
}

LayoutPoint TextLayout::CursorPosition(std::size_t index) const {
    if (points_.empty()) return {};
    return points_[std::min(index, points_.size() - 1)];
}

std::size_t TextLayout::LineFor(std::size_t index) const {
    if (line_for_.empty()) return 0;
    return line_for_[std::min(index, line_for_.size() - 1)];
}

std::pair<std::size_t, std::size_t> TextLayout::VisibleLines(float scroll, float height) const {
    if (lines_.empty()) return {0, 0};
    const auto count = lines_.size();
    const auto first = static_cast<std::size_t>(std::clamp(
        std::floor(static_cast<double>(scroll / line_height_)) - 1, 0.0, static_cast<double>(count)));
    const auto end = static_cast<std::size_t>(std::clamp(
        std::ceil(static_cast<double>((scroll + height) / line_height_)) + 1, 0.0, static_cast<double>(count)));
    return {first, std::max(first, end)};
}

float TextLayout::CenteredScroll(std::size_t line, float viewport_height) const {
    if (lines_.empty()) return 0;
    const auto& target = lines_[std::min(line, lines_.size() - 1)];
    return target.y + line_height_ * 0.5f - viewport_height * 0.5f;
}

}  // namespace typist
