#include "text_viewport.h"
#include <rlgl.h>

#include <algorithm>
#include <cmath>

namespace typist {

void TextViewport::Shutdown() {
    if (drawing_) End();
    if (target_texture_.id) UnloadRenderTexture(target_texture_);
    target_texture_ = {};
    width_ = height_ = 0;
}

void TextViewport::Configure(float logical_width, float logical_height, float dpi_x, float dpi_y) {
    dpi_x_ = std::max(0.1f, dpi_x);
    dpi_y_ = std::max(0.1f, dpi_y);
    logical_height_ = std::max(1.0f, logical_height);
    const int width = std::max(1, static_cast<int>(std::round(logical_width * dpi_x_)));
    const int height = std::max(1, static_cast<int>(std::round(logical_height_ * dpi_y_)));
    if (width == width_ && height == height_) return;
    Shutdown();
    width_ = width;
    height_ = height;
    target_texture_ = LoadRenderTexture(width, height);
}

void TextViewport::Follow(const TextLayout& layout, std::size_t cursor, bool instant) {
    if (browsing_) return;
    target_ = layout.CenteredScroll(layout.LineFor(cursor), Height());
    if (instant || std::abs(target_ - scroll_) > Height() * 1.5f) scroll_ = target_;
}

void TextViewport::Browse(float lines, const TextLayout& layout) {
    if (layout.Height() <= Height()) return;
    browsing_ = true;
    const float low = layout.CenteredScroll(0, Height());
    const float high = layout.CenteredScroll(layout.Lines().size() - 1, Height());
    target_ = std::clamp(target_ + lines * layout.LineHeight(), low, high);
}

void TextViewport::Return(const TextLayout& layout, std::size_t cursor, bool instant) {
    browsing_ = false;
    Follow(layout, cursor, instant);
}

void TextViewport::Update(float dt, bool reduced_motion) {
    if (reduced_motion) scroll_ = target_;
    else scroll_ += (target_ - scroll_) * (1.0f - std::exp(-std::max(0.0f, dt) * 13.0f));
}

bool TextViewport::Begin() {
    if (!Valid()) return false;
    BeginTextureMode(target_texture_);
    ClearBackground(BLANK);
    rlPushMatrix();
    rlScalef(dpi_x_, dpi_y_, 1);
    drawing_ = true;
    return true;
}

void TextViewport::End() {
    if (!drawing_) return;
    rlPopMatrix();
    EndTextureMode();
    drawing_ = false;
}

}  // namespace typist
