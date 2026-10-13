#pragma once

#include "text_layout.h"
#include <raylib.h>

namespace typist {

class TextViewport {
public:
    ~TextViewport() { Shutdown(); }
    void Shutdown();
    void Configure(float width, float height, float dpi_x, float dpi_y);
    void Follow(const TextLayout& layout, std::size_t cursor, bool instant);
    void Browse(float lines, const TextLayout& layout);
    void Return(const TextLayout& layout, std::size_t cursor, bool instant);
    void Update(float dt, bool reduced_motion);
    bool Browsing() const { return browsing_; }
    float Scroll() const { return scroll_; }
    float Target() const { return target_; }
    float Height() const { return logical_height_; }
    int Width() const { return width_; }
    bool Begin();
    void End();
    Texture2D Texture() const { return target_texture_.texture; }
    bool Valid() const { return IsRenderTextureValid(target_texture_); }

private:
    RenderTexture2D target_texture_{};
    int width_ = 0;
    int height_ = 0;
    float logical_height_ = 1;
    float dpi_x_ = 1;
    float dpi_y_ = 1;
    float scroll_ = 0;
    float target_ = 0;
    bool browsing_ = false;
    bool drawing_ = false;
};

}  // namespace typist
