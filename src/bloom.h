#pragma once

#include <raylib.h>

#include <array>

namespace typist {

// A bright pass is downsampled four times, then rebuilt from small to large.
// Call Begin and EndAndComposite inside one BeginDrawing/EndDrawing pair.
class Bloom {
public:
    bool Begin(int logical_width, int logical_height, float dpi_x, float dpi_y);
    void EndAndComposite(float strength);
    void Shutdown();

private:
    static constexpr int kLevels = 4;
    bool EnsureSize(int width, int height);
    RenderTexture2D scene_{};
    std::array<RenderTexture2D, kLevels> down_{};
    std::array<RenderTexture2D, kLevels> up_{};
    Shader downsample_{};
    int texel_location_ = -1;
    int threshold_location_ = -1;
    int width_ = 0;
    int height_ = 0;
    bool drawing_ = false;
};

}  // namespace typist
