#include "bloom.h"
#include <rlgl.h>

#include <algorithm>
#include <cmath>

namespace typist {

namespace {

constexpr const char* kDownsampleFragment = R"glsl(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 u_texel;
uniform float u_threshold;
out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;
    vec3 color = texture(texture0, uv).rgb * 0.25;
    color += texture(texture0, uv + u_texel * vec2(-1.0, -1.0)).rgb * 0.1875;
    color += texture(texture0, uv + u_texel * vec2( 1.0, -1.0)).rgb * 0.1875;
    color += texture(texture0, uv + u_texel * vec2(-1.0,  1.0)).rgb * 0.1875;
    color += texture(texture0, uv + u_texel * vec2( 1.0,  1.0)).rgb * 0.1875;
    // Preserve saturated neon hues: a luminance cutoff suppresses dark panels
    // without discarding bright red/blue channels individually.
    float peak = max(color.r, max(color.g, color.b));
    color *= smoothstep(u_threshold, u_threshold + 0.20, peak);
    finalColor = vec4(color, 1.0);
}
)glsl";

void DrawTarget(Texture2D texture, float width, float height, Color tint = WHITE) {
    DrawTexturePro(texture,
                   {0, 0, static_cast<float>(texture.width), -static_cast<float>(texture.height)},
                   {0, 0, width, height}, {0, 0}, 0, tint);
}

}  // namespace

bool Bloom::EnsureSize(int width, int height) {
    if (width == width_ && height == height_ && IsRenderTextureValid(scene_)
        && IsShaderValid(downsample_)) return true;
    Shutdown();
    if (width < 1 || height < 1) return false;

    scene_ = LoadRenderTexture(width, height);
    int level_width = width;
    int level_height = height;
    for (int i = 0; i < kLevels; ++i) {
        level_width = std::max(1, level_width / 2);
        level_height = std::max(1, level_height / 2);
        down_[i] = LoadRenderTexture(level_width, level_height);
        up_[i] = LoadRenderTexture(level_width, level_height);
        SetTextureFilter(down_[i].texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(up_[i].texture, TEXTURE_FILTER_BILINEAR);
    }
    downsample_ = LoadShaderFromMemory(nullptr, kDownsampleFragment);
    if (!IsRenderTextureValid(scene_) || !IsShaderValid(downsample_)) {
        Shutdown();
        return false;
    }
    for (int i = 0; i < kLevels; ++i) {
        if (!IsRenderTextureValid(down_[i]) || !IsRenderTextureValid(up_[i])) {
            Shutdown();
            return false;
        }
    }
    texel_location_ = GetShaderLocation(downsample_, "u_texel");
    threshold_location_ = GetShaderLocation(downsample_, "u_threshold");
    width_ = width;
    height_ = height;
    return true;
}

bool Bloom::Begin(int logical_width, int logical_height, float dpi_x, float dpi_y) {
    const int width = std::max(1, static_cast<int>(std::round(logical_width * dpi_x)));
    const int height = std::max(1, static_cast<int>(std::round(logical_height * dpi_y)));
    if (!EnsureSize(width, height)) return false;
    BeginTextureMode(scene_);
    rlPushMatrix();
    rlScalef(static_cast<float>(width) / logical_width,
             static_cast<float>(height) / logical_height, 1);
    drawing_ = true;
    return true;
}

void Bloom::EndAndComposite(float strength) {
    if (!drawing_) return;
    rlPopMatrix();
    EndTextureMode();
    drawing_ = false;

    Texture2D source = scene_.texture;
    for (int i = 0; i < kLevels; ++i) {
        const float texel[2] = {1.0f / source.width, 1.0f / source.height};
        const float threshold = i == 0 ? 0.34f : 0.0f;
        SetShaderValue(downsample_, texel_location_, texel, SHADER_UNIFORM_VEC2);
        SetShaderValue(downsample_, threshold_location_, &threshold, SHADER_UNIFORM_FLOAT);
        BeginTextureMode(down_[i]);
        ClearBackground(BLANK);
        BeginShaderMode(downsample_);
        DrawTarget(source, static_cast<float>(down_[i].texture.width),
                   static_cast<float>(down_[i].texture.height));
        EndShaderMode();
        EndTextureMode();
        source = down_[i].texture;
    }

    for (int i = kLevels - 1; i >= 0; --i) {
        const float w = static_cast<float>(up_[i].texture.width);
        const float h = static_cast<float>(up_[i].texture.height);
        BeginTextureMode(up_[i]);
        ClearBackground(BLANK);
        DrawTarget(down_[i].texture, w, h);
        if (i + 1 < kLevels) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawTarget(up_[i + 1].texture, w, h, {255, 255, 255, 166});
            EndBlendMode();
        }
        EndTextureMode();
    }

    ClearBackground(BLACK);
    DrawTarget(scene_.texture, static_cast<float>(width_), static_cast<float>(height_));
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTarget(up_[0].texture, static_cast<float>(width_), static_cast<float>(height_),
               {255, 255, 255, static_cast<unsigned char>(std::clamp(strength, 0.0f, 1.0f) * 255)});
    EndBlendMode();
}

void Bloom::Shutdown() {
    if (drawing_) {
        rlPopMatrix();
        EndTextureMode();
        drawing_ = false;
    }
    if (scene_.id) UnloadRenderTexture(scene_);
    for (auto& target : down_) if (target.id) UnloadRenderTexture(target);
    for (auto& target : up_) if (target.id) UnloadRenderTexture(target);
    if (downsample_.id) UnloadShader(downsample_);
    scene_ = {};
    down_ = {};
    up_ = {};
    downsample_ = {};
    width_ = 0;
    height_ = 0;
}

}  // namespace typist
