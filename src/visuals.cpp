#include "visuals.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace typist {

namespace {
float Random01() { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); }
Color WithAlpha(Color c, float a) {
    c.a = static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
    return c;
}
}  // namespace

void Visuals::CyclePreset() {
    preset_ = static_cast<Preset>((static_cast<int>(preset_) + 1) % static_cast<int>(Preset::count));
    for (auto& particle : particles_) particle.life = 0;
    impact_age_ = 1;
}

const char* Visuals::Name() const {
    switch (preset_) {
        case Preset::focus: return "FOCUS";
        case Preset::neon: return "NEON";
        case Preset::aurora: return "AURORA";
        case Preset::overdrive: return "OVERDRIVE";
        case Preset::minimal: return "MINIMAL";
        case Preset::count: break;
    }
    return "FOCUS";
}

Color Visuals::Accent() const {
    switch (preset_) {
        case Preset::focus: return Color{35, 123, 101, 255};
        case Preset::neon: return Color{50, 245, 178, 255};
        case Preset::aurora: return Color{192, 158, 255, 255};
        case Preset::overdrive: return Color{255, 207, 67, 255};
        case Preset::minimal: return Color{19, 29, 29, 255};
        case Preset::count: break;
    }
    return WHITE;
}

Color Visuals::TextColor() const {
    if (preset_ == Preset::focus) return Color{43, 49, 48, 255};
    if (preset_ == Preset::minimal) return Color{27, 35, 35, 255};
    if (preset_ == Preset::overdrive) return Color{252, 242, 221, 255};
    return Color{232, 240, 246, 255};
}

Color Visuals::MutedColor() const {
    if (preset_ == Preset::focus) return Color{91, 105, 100, 255};
    if (preset_ == Preset::minimal) return Color{120, 130, 129, 255};
    if (preset_ == Preset::overdrive) return Color{184, 166, 160, 255};
    return Color{152, 170, 189, 255};
}

void Visuals::Burst(Vector2 origin, bool success) {
    if (preset_ == Preset::focus || preset_ == Preset::minimal || reduced_motion_) return;
    if (preset_ == Preset::overdrive) {
        impact_origin_ = origin;
        impact_age_ = 0;
        impact_color_ = success ? Accent() : Color{255, 83, 122, 255};
    }
    const int count = preset_ == Preset::overdrive ? 24 : 10;
    for (int i = 0; i < count; ++i) {
        Particle& p = particles_[next_particle_++ % particles_.size()];
        const float angle = Random01() * 6.2831853f;
        const float speed = 45 + Random01() * (preset_ == Preset::overdrive ? 360 : 170);
        p.position = origin;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.color = success ? Accent() : Color{255, 107, 125, 255};
        p.age = 0;
        p.life = 0.35f + Random01() * 0.65f;
        p.radius = 1.5f + Random01() * 3.5f;
    }
}

void Visuals::Update(float dt) {
    dt = std::clamp(dt, 0.0f, 0.05f);
    impact_age_ = std::min(1.0f, impact_age_ + dt * 2.4f);
    for (auto& p : particles_) {
        if (p.age >= p.life) continue;
        p.age += dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.velocity.x *= 1.0f - 1.7f * dt;
        p.velocity.y = p.velocity.y * (1.0f - 1.7f * dt) + 50 * dt;
    }
}

void Visuals::DrawBackground(int width, int height, float time) const {
    if (reduced_motion_) time = 0;
    switch (preset_) {
        case Preset::focus:
            ClearBackground(Color{244, 242, 235, 255});
            for (int y = 0; y < height; y += 42)
                DrawLine(0, y, width, y, Color{232, 232, 222, 255});
            break;
        case Preset::neon:
            ClearBackground(Color{5, 19, 24, 255});
            for (int x = 0; x < width; x += 48)
                DrawLine(x, 0, x, height, Color{13, 43, 47, 255});
            for (int y = 0; y < height; y += 48)
                DrawLine(0, y, width, y, Color{13, 43, 47, 255});
            DrawRectangle(0, 0, 8, height, Accent());
            break;
        case Preset::aurora:
            ClearBackground(Color{24, 18, 48, 255});
            for (int i = 0; i < 7; ++i) {
                const float x = width * (0.1f + i * 0.14f);
                const float y = height * (0.18f + (i % 3) * 0.10f);
                const Color color = i % 2 ? Color{65, 215, 205, 255} : Color{177, 105, 249, 255};
                DrawCircleGradient({x, y}, width * 0.30f,
                                   WithAlpha(color, 0.18f), WithAlpha(color, 0));
            }
            break;
        case Preset::overdrive:
            ClearBackground(Color{31, 14, 24, 255});
            for (int i = -height; i < width + height; i += 110) {
                const float shift = reduced_motion_ ? 0 : std::fmod(time * 55, 110.0f);
                DrawLineEx({static_cast<float>(i) + shift, 0},
                           {static_cast<float>(i + height) + shift, static_cast<float>(height)},
                           22, Color{68, 25, 42, 255});
            }
            DrawRectangle(0, 0, width, 10, Color{255, 71, 104, 255});
            break;
        case Preset::minimal:
            ClearBackground(Color{249, 249, 246, 255});
            break;
        case Preset::count:
            break;
    }
}

void Visuals::DrawExerciseSurface(Rectangle bounds, float time) const {
    if (reduced_motion_) time = 0;
    switch (preset_) {
        case Preset::focus:
            DrawRectangleRounded(bounds, 0.035f, 8, Color{255, 255, 252, 255});
            DrawRectangle(static_cast<int>(bounds.x), static_cast<int>(bounds.y), 4,
                          static_cast<int>(bounds.height), Accent());
            break;
        case Preset::neon: {
            DrawRectangleRec(bounds, Color{5, 12, 20, 245});
            const float corner = 28;
            const Color c = Accent();
            for (int side = 0; side < 2; ++side) {
                const float x = side ? bounds.x + bounds.width : bounds.x;
                const float sx = side ? -corner : corner;
                DrawLineEx({x, bounds.y}, {x + sx, bounds.y}, 3, c);
                DrawLineEx({x, bounds.y}, {x, bounds.y + corner}, 3, c);
                DrawLineEx({x, bounds.y + bounds.height}, {x + sx, bounds.y + bounds.height}, 3, c);
                DrawLineEx({x, bounds.y + bounds.height}, {x, bounds.y + bounds.height - corner}, 3, c);
            }
            const float scan = bounds.y + std::fmod(time * 55, bounds.height);
            DrawLineEx({bounds.x + 3, scan}, {bounds.x + bounds.width - 3, scan},
                       1, WithAlpha(c, 0.12f));
            break;
        }
        case Preset::aurora:
            DrawRectangleRounded(bounds, 0.22f, 24, Color{37, 30, 72, 225});
            DrawRectangleRoundedLinesEx(bounds, 0.22f, 24, 2,
                                        WithAlpha(Color{190, 184, 255, 255}, 0.4f));
            break;
        case Preset::overdrive:
            DrawRectangleRec({bounds.x + 12, bounds.y + 12, bounds.width, bounds.height},
                             Color{255, 71, 104, 255});
            DrawRectangleRec(bounds, Color{51, 20, 35, 255});
            DrawRectangleRec({bounds.x, bounds.y, bounds.width, 8}, Accent());
            break;
        case Preset::minimal:
            break;
        case Preset::count:
            break;
    }
}

void Visuals::DrawProgress(Rectangle bounds, float progress) const {
    progress = std::clamp(progress, 0.0f, 1.0f);
    const Color track = preset_ == Preset::focus ? Color{209, 218, 210, 255}
                      : preset_ == Preset::neon ? Color{23, 64, 63, 255}
                      : preset_ == Preset::aurora ? Color{82, 69, 119, 255}
                      : Color{91, 47, 58, 255};
    DrawRectangleRec(bounds, track);
    if (progress > 0) DrawRectangleRec({bounds.x, bounds.y, bounds.width * progress, bounds.height}, Accent());
    if (preset_ == Preset::overdrive && progress > 0) {
        const float marker = bounds.x + bounds.width * progress;
        DrawTriangle({marker, bounds.y - 8}, {marker - 6, bounds.y - 16},
                     {marker + 6, bounds.y - 16}, Accent());
    }
}

void Visuals::DrawForeground(int width, int height, float time) const {
    (void)width; (void)height; (void)time;
    if (preset_ == Preset::focus || preset_ == Preset::minimal || reduced_motion_) return;
    if (preset_ == Preset::overdrive && impact_age_ < 1) {
        const float radius = 18 + 105 * impact_age_;
        const float alpha = (1 - impact_age_) * 0.68f;
        DrawCircleLinesV(impact_origin_, radius, WithAlpha(impact_color_, alpha));
        DrawCircleLinesV(impact_origin_, radius * 0.65f, WithAlpha(impact_color_, alpha * 0.45f));
        for (int i = 0; i < 8; ++i) {
            const float angle = i * 0.78539816f;
            const float inner = radius + 8;
            const float outer = inner + 22 * (1 - impact_age_);
            DrawLineEx({impact_origin_.x + std::cos(angle) * inner,
                        impact_origin_.y + std::sin(angle) * inner},
                       {impact_origin_.x + std::cos(angle) * outer,
                        impact_origin_.y + std::sin(angle) * outer},
                       3, WithAlpha(impact_color_, alpha));
        }
    }
    for (const auto& p : particles_) {
        if (p.life <= 0 || p.age >= p.life) continue;
        const float alpha = 1.0f - p.age / p.life;
        DrawCircleV(p.position, p.radius * (0.5f + alpha), WithAlpha(p.color, alpha * 0.8f));
    }
}

}  // namespace typist
