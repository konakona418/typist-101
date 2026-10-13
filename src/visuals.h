#pragma once

#include <raylib.h>

#include <array>
#include <cstddef>

namespace typist {

enum class Preset { focus, neon, aurora, overdrive, minimal, count };

struct Particle {
    Vector2 position{};
    Vector2 velocity{};
    Color color{};
    float age = 0;
    float life = 0;
    float radius = 0;
};

class Visuals {
public:
    void CyclePreset();
    void SelectPreset(Preset preset) { if (preset != Preset::count) preset_ = preset; }
    Preset GetPreset() const { return preset_; }
    void SetReducedMotion(bool enabled) { reduced_motion_ = enabled; }
    bool ReducedMotion() const { return reduced_motion_; }
    const char* Name() const;
    void Update(float dt);
    void Burst(Vector2 origin, bool success);
    void DrawBackground(int width, int height, float time) const;
    void DrawExerciseSurface(Rectangle bounds, float time) const;
    void DrawProgress(Rectangle bounds, float progress) const;
    void DrawForeground(int width, int height, float time) const;
    Color Accent() const;
    Color TextColor() const;
    Color MutedColor() const;

private:
    Preset preset_ = Preset::focus;
    bool reduced_motion_ = false;
    std::array<Particle, 260> particles_{};
    std::size_t next_particle_ = 0;
    Vector2 impact_origin_{};
    float impact_age_ = 1;
    Color impact_color_{};
};

}  // namespace typist
