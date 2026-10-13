#include "bloom.h"
#include "font.h"
#include "lesson.h"
#include "progress.h"
#include "session.h"
#include "slug_text.h"
#include "status.h"
#include "text_layout.h"
#include "text_input.h"
#include "text_viewport.h"
#include "utf8.h"
#include "visuals.h"

#include <raylib.h>
#include <rlgl.h>
#ifdef TYPIST_SDL3
#include <SDL3/SDL.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {

struct Lesson {
    const char* title;
    const char* text;
};

constexpr std::array<Lesson, 5> kLessons{{
    {"01 / HOME ROW", "asdf jkl; asdf jkl; sad lad fall ask"},
    {"02 / EASY WORDS", "the sun is warm and the air is calm"},
    {"03 / RHYTHM", "slow and steady, one key at a time"},
    {"04 / SENTENCE", "A small step today is still a step forward."},
    {"05 / NUMBERS", "123 456 789 2026 10 13"},
}};

Color FadeTo(Color c, float alpha) {
    c.a = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 1.0f) * 255);
    return c;
}

void Label(const typist::SlugText& font, std::string_view text, float x, float y,
           float size, Color color) {
    font.Draw(text, {x, y}, size, color);
}

bool HasGlyph(Font font, char32_t codepoint) {
    for (int i = 0; i < font.glyphCount; ++i)
        if (font.glyphs[i].value == static_cast<int>(codepoint)) return true;
    return false;
}

bool IsMinimal(const typist::Visuals& visuals) {
    return visuals.GetPreset() == typist::Preset::minimal;
}

std::unique_ptr<typist::Session> NewSession(const std::vector<std::string>& texts,
                                            std::size_t lesson_index,
                                            const typist::Visuals& visuals) {
    if (IsMinimal(visuals))
    {
        std::string line = texts[lesson_index];
        std::replace(line.begin(), line.end(), '\n', ' ');
        return std::make_unique<typist::Session>(line + " ", true);
    }
    return std::make_unique<typist::Session>(texts[lesson_index]);
}

void DrawExercise(const typist::SlugText& font, const typist::Session& session, const typist::Snapshot& snapshot,
                  const typist::Visuals& visuals, const typist::TextLayout& layout, float scroll, float height,
                  float size, float time, float mistake_flash) {
    const auto& text = session.Text();
    const auto preset = visuals.GetPreset();
    const auto accent = visuals.Accent();
    const bool glow = preset == typist::Preset::neon || preset == typist::Preset::overdrive;
    const auto [first, last] = layout.VisibleLines(scroll, height);
    for (std::size_t line = first; line < last; ++line) for (std::size_t i = layout.Lines()[line].begin;
         i < layout.Lines()[line].end; ++i) {
        const auto glyph = layout.CursorPosition(i);
        const Vector2 point{glyph.x, glyph.y - scroll};
        const Color tint = i < snapshot.cursor ? accent : visuals.TextColor();
        if (glow && i < snapshot.cursor) {
            for (int j = 4; j >= 1; --j) {
                font.Draw(text[i], {point.x + j * 0.5f, point.y}, size,
                          FadeTo(accent, (preset == typist::Preset::overdrive ? 0.12f : 0.08f) * j));
            }
        }
        font.Draw(text[i], point, size, tint);
    }
    for (std::size_t line = first; line < last; ++line) {
        const auto end = layout.Lines()[line].end;
        if (end < text.size() && text[end] == U'\n' && end >= snapshot.cursor &&
            layout.LineFor(end) == line) {
            const auto p = layout.CursorPosition(end);
            font.Draw("ENTER", {p.x + 12, p.y - scroll + size * 0.28f},
                      size * 0.42f, visuals.MutedColor());
        }
    }
    if (!snapshot.completed && static_cast<int>(time * 2.0f) % 2 == 0) {
        const auto cursor = layout.CursorPosition(snapshot.cursor);
        DrawRectangleRounded({cursor.x - 2, cursor.y - scroll + size * 0.16f, 3, size * 0.86f},
                             0.5f, 8, mistake_flash > 0 ? Color{255, 111, 130, 255} : accent);
    }
    if (mistake_flash > 0 && !snapshot.completed) {
        const auto cursor = layout.CursorPosition(snapshot.cursor);
        DrawCircleGradient({cursor.x, cursor.y - scroll + size * 0.5f},
                           45, FadeTo(Color{255, 70, 110, 255}, mistake_flash * 0.32f),
                           FadeTo(RED, 0));
    }
}

void DrawMinimalExercise(const typist::SlugText& font, const typist::Session& session,
                         const typist::Snapshot& snapshot, int width, int height,
                         float mistake_flash) {
    const auto& text = session.Text();
    if (text.empty()) return;
    const float size = std::clamp(width * 0.078f, 58.0f, 110.0f);
    const float cursor_x = width * 0.28f;
    const float baseline_y = height * 0.49f;
    const std::size_t count = text.size();
    const std::size_t cursor = snapshot.cursor;
    const std::size_t start = cursor > 14 ? cursor - 14 : 0;
    float x = cursor_x;
    for (std::size_t i = start; i < cursor; ++i)
        x -= font.Measure(text[i % count], size);

    BeginScissorMode(0, static_cast<int>(baseline_y - size * 0.6f), width,
                     static_cast<int>(size * 2.25f));
    const std::size_t visible = std::max<std::size_t>(count * 2, 50);
    for (std::size_t i = start; i < cursor + visible && x < width + size; ++i) {
        const char32_t codepoint = text[i % count];
        const float advance = font.Measure(codepoint, size);
        if (x + advance >= -size)
            font.Draw(codepoint, {x, baseline_y}, size,
                      i < cursor ? Color{175, 184, 180, 255} : Color{27, 35, 35, 255});
        x += advance;
    }
    EndScissorMode();

    const Color cursor_color = mistake_flash > 0 ? Color{220, 67, 77, 255} : Color{24, 106, 83, 255};
    DrawRectangle(static_cast<int>(cursor_x - 3), static_cast<int>(baseline_y + size * 0.11f),
                  5, static_cast<int>(size * 0.95f), cursor_color);
}

}  // namespace

int main(int argc, char** argv) {
    int initial_width = 1280;
    int initial_height = 800;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string_view(argv[i]) != "--window-size") continue;
        int requested_width = 0;
        int requested_height = 0;
        if (std::sscanf(argv[i + 1], "%dx%d", &requested_width, &requested_height) == 2) {
            initial_width = std::max(760, requested_width);
            initial_height = std::max(540, requested_height);
        }
    }
#ifdef TYPIST_SDL3
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
#endif
    InitWindow(initial_width, initial_height, "typist 101");
    SetExitKey(KEY_NULL);
    SetWindowMinSize(760, 540);
    SetTargetFPS(60);
#ifdef TYPIST_SDL_IME
    typist::TextInput text_input(GetWindowHandle());
#else
    typist::TextInput text_input;
#endif

    std::vector<std::string> texts;
    texts.reserve(kLessons.size());
    for (const auto& lesson : kLessons) texts.emplace_back(lesson.text);
    std::vector<std::string> titles;
    std::vector<std::string> lesson_ids;
    for (const auto& lesson : kLessons) {
        titles.emplace_back(lesson.title);
        lesson_ids.emplace_back(lesson.title);
    }
    auto font_texts = [&]() {
        auto all = texts;
        all.insert(all.end(), titles.begin(), titles.end());
        return all;
    };
    Font font = typist::LoadExerciseFont(font_texts(), 120);
    auto slug_text = std::make_unique<typist::SlugText>(font_texts(), font);
    Font preedit_font = GetFontDefault();
    bool preedit_owns_font = false;
    std::string cached_preedit;
    auto clear_preedit_font = [&]() {
        if (preedit_owns_font) UnloadFont(preedit_font);
        preedit_font = GetFontDefault();
        preedit_owns_font = false;
        cached_preedit.clear();
    };

    std::size_t lesson_index = 0;
    typist::Visuals visuals;
    std::string screenshot_path;
    std::vector<std::string> startup_lessons;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--preset" && i + 1 < argc) {
            const std::string name = argv[++i];
            if (name == "focus") visuals.SelectPreset(typist::Preset::focus);
            else if (name == "neon") visuals.SelectPreset(typist::Preset::neon);
            else if (name == "aurora") visuals.SelectPreset(typist::Preset::aurora);
            else if (name == "overdrive") visuals.SelectPreset(typist::Preset::overdrive);
            else if (name == "minimal") visuals.SelectPreset(typist::Preset::minimal);
        } else if (arg == "--screenshot" && i + 1 < argc) {
            screenshot_path = argv[++i];
        } else if (arg == "--lesson" && i + 1 < argc) {
            startup_lessons.emplace_back(argv[++i]);
        }
    }
    std::string notice;
    auto load_custom = [&](const std::string& path) {
        const auto loaded = typist::LoadLessonFile(path);
        if (!loaded) { notice = loaded.error; return false; }
        clear_preedit_font();
        slug_text->Shutdown();
        if (font.texture.id != GetFontDefault().texture.id) UnloadFont(font);
        texts.push_back(loaded.text);
        titles.push_back(std::filesystem::path(path).filename().string());
        lesson_ids.push_back(std::filesystem::absolute(path).lexically_normal().string());
        font = typist::LoadExerciseFont(font_texts(), 120);
        slug_text = std::make_unique<typist::SlugText>(font_texts(), font);
        lesson_index = texts.size() - 1;
        notice.clear();
        return true;
    };
    for (const auto& path : startup_lessons) load_custom(path);
    auto session = NewSession(texts, lesson_index, visuals);
    auto progress_path = [&]() { return typist::ProgressPath(lesson_ids[lesson_index], texts[lesson_index]); };
    auto restore = [&]() {
        if (IsMinimal(visuals)) return;
        typist::SessionState saved;
        if (typist::ReadProgress(progress_path(), texts[lesson_index], saved))
            session->Restore(saved, GetTime());
    };
    bool progress_dirty = false;
    double last_progress_write = GetTime();
    auto save = [&]() {
        if (IsMinimal(visuals)) return;
        if (typist::WriteProgress(progress_path(), texts[lesson_index], session->Save(GetTime()))) {
            progress_dirty = false;
            last_progress_write = GetTime();
        }
    };
    restore();
    typist::Bloom bloom;
    typist::TextLayout layout;
    typist::TextViewport viewport;
    int layout_width = 0;
    float layout_size = 0;
    bool layout_dirty = true;
    float mistake_flash = 0;
    float success_flash = 0;
    int rendered_frames = 0;

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        const float time = static_cast<float>(GetTime());
        const int width = GetScreenWidth();
        const int height = GetScreenHeight();
        const float margin = std::max(42.0f, width * 0.10f);
        const float exercise_top = height * 0.37f;
        const float exercise_size = std::clamp(width * 0.046f, 44.0f, 68.0f);
        const float progress_y = std::min(height * 0.70f, height - 180.0f);
        const float exercise_height = std::min(height * 0.32f, progress_y - exercise_top - 10.0f);
        const Rectangle exercise_view{margin, exercise_top, width - 2 * margin, exercise_height};
        if (IsFileDropped()) {
            const auto files = LoadDroppedFiles();
            if (files.count > 0) {
                save();
            }
            if (files.count > 0 && load_custom(files.paths[0])) {
                session = NewSession(texts, lesson_index, visuals);
                restore();
                layout_dirty = true;
            }
            UnloadDroppedFiles(files);
        }
        float dpi_x = static_cast<float>(GetRenderWidth()) / std::max(1, width);
        float dpi_y = static_cast<float>(GetRenderHeight()) / std::max(1, height);
#ifdef TYPIST_SDL3
        if (SDL_Window* window = SDL_GetKeyboardFocus()) {
            int pixel_width = 0;
            int pixel_height = 0;
            SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height);
            if (pixel_width > 0 && pixel_height > 0) {
                dpi_x = static_cast<float>(pixel_width) / std::max(1, width);
                dpi_y = static_cast<float>(pixel_height) / std::max(1, height);
            }
        }
#endif
        const bool composing = text_input.ConsumeCompositionActivity();
        viewport.Configure(exercise_view.width, exercise_view.height, dpi_x, dpi_y);
        if (layout_dirty || layout_width != static_cast<int>(exercise_view.width) || layout_size != exercise_size) {
            const float newline_hint_width = slug_text->Measure("ENTER", exercise_size * 0.42f) + 12;
            layout.Build(session->Text(), exercise_view.width, exercise_size * 1.62f,
                         [&](char32_t c) { return slug_text->Measure(c, exercise_size); },
                         newline_hint_width);
            layout_width = static_cast<int>(exercise_view.width);
            layout_size = exercise_size;
            const auto current = session->GetSnapshot(time);
            viewport.Return(layout, current.cursor, true);
            layout_dirty = false;
        }

        if (IsKeyPressed(KEY_F1)) {
            const bool was_minimal = IsMinimal(visuals);
            if (!was_minimal) save();
            visuals.CyclePreset();
            if (was_minimal != IsMinimal(visuals))
                session = NewSession(texts, lesson_index, visuals);
            if (was_minimal && !IsMinimal(visuals)) restore();
            if (was_minimal != IsMinimal(visuals)) layout_dirty = true;
        }
        if (IsKeyPressed(KEY_F6)) visuals.SetReducedMotion(!visuals.ReducedMotion());
        if (!composing && (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_TAB))) {
            save();
            lesson_index = (lesson_index + 1) % texts.size();
            session = NewSession(texts, lesson_index, visuals);
            restore();
            layout_dirty = true;
        }
        if (!composing && IsKeyPressed(KEY_LEFT)) {
            save();
            lesson_index = (lesson_index + texts.size() - 1) % texts.size();
            session = NewSession(texts, lesson_index, visuals);
            restore();
            layout_dirty = true;
        }
        if (IsKeyPressed(KEY_F5)) { session = NewSession(texts, lesson_index, visuals); save(); layout_dirty = true; }
        if (layout_dirty) {
            const float newline_hint_width = slug_text->Measure("ENTER", exercise_size * 0.42f) + 12;
            layout.Build(session->Text(), exercise_view.width, exercise_size * 1.62f,
                         [&](char32_t c) { return slug_text->Measure(c, exercise_size); },
                         newline_hint_width);
            const auto current = session->GetSnapshot(time);
            viewport.Return(layout, current.cursor, true);
            layout_dirty = false;
        }
        if (!IsMinimal(visuals)) {
            const float wheel = GetMouseWheelMove();
            if (wheel != 0) viewport.Browse(-wheel * 2, layout);
            if (IsKeyPressed(KEY_PAGE_UP)) viewport.Browse(-3, layout);
            if (IsKeyPressed(KEY_PAGE_DOWN)) viewport.Browse(3, layout);
            if (!composing && IsKeyPressed(KEY_ESCAPE)) {
                const auto current = session->GetSnapshot(time);
                viewport.Return(layout, current.cursor, true);
            }
        }
        if (!composing && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))) {
            const auto current = session->GetSnapshot(time);
            viewport.Return(layout, current.cursor, true);
            if (session->Backspace() == typist::InputResult::erased) progress_dirty = true;
        }

        bool enter_handled = false;
        if (!IsMinimal(visuals) && session->GetSnapshot(time).cursor < session->Text().size() &&
            session->Text()[session->GetSnapshot(time).cursor] == U'\n' &&
            !composing && IsKeyPressed(KEY_ENTER)) {
            const auto current = session->GetSnapshot(time);
            viewport.Return(layout, current.cursor, true);
            const auto result = session->Input(U'\n', GetTime());
            if (result == typist::InputResult::completed) success_flash = 1.0f;
            if (result == typist::InputResult::completed) save();
            else if (result != typist::InputResult::ignored) progress_dirty = true;
            enter_handled = true;
        }

        std::vector<char32_t> committed;
#ifdef TYPIST_SDL_IME
        for (const auto& chunk : text_input.TakeCommitted()) {
            const auto points = typist::DecodeUtf8(chunk);
            committed.insert(committed.end(), points.begin(), points.end());
        }
#else
        while (const int codepoint = GetCharPressed()) committed.push_back(static_cast<char32_t>(codepoint));
#endif
        for (char32_t codepoint : committed) {
            if (codepoint == U'\r' || (enter_handled && codepoint == U'\n')) continue;
            const auto current = session->GetSnapshot(time);
            viewport.Return(layout, current.cursor, true);
            const auto result = session->Input(codepoint, GetTime());
            if (result == typist::InputResult::completed) save();
            else if (result != typist::InputResult::ignored) progress_dirty = true;
            if (result == typist::InputResult::mistake) mistake_flash = 0.75f;
            if (result == typist::InputResult::completed) success_flash = 1.0f;
            if (IsMinimal(visuals)) continue;
            if (result == typist::InputResult::correct || result == typist::InputResult::completed) {
                const auto cursor = session->GetSnapshot(GetTime()).cursor;
                const auto p = layout.CursorPosition(cursor - 1);
                const float local_y = p.y - viewport.Scroll();
                if (local_y >= 0 && local_y < exercise_view.height)
                    visuals.Burst({exercise_view.x + p.x, exercise_view.y + local_y}, true);
            } else if (result == typist::InputResult::mistake) {
                const auto cursor = session->GetSnapshot(GetTime()).cursor;
                const auto p = layout.CursorPosition(cursor);
                const float local_y = p.y - viewport.Scroll();
                if (local_y >= 0 && local_y < exercise_view.height)
                    visuals.Burst({exercise_view.x + p.x, exercise_view.y + local_y}, false);
            }
        }

        mistake_flash = std::max(0.0f, mistake_flash - dt * 2.4f);
        success_flash = std::max(0.0f, success_flash - dt * 0.7f);
        visuals.Update(dt);
        if (progress_dirty && GetTime() - last_progress_write >= 0.75) save();
        const auto snapshot = session->GetSnapshot(GetTime());
        if (cached_preedit != text_input.Preedit()) {
            clear_preedit_font();
            cached_preedit = text_input.Preedit();
            if (!cached_preedit.empty()) {
                slug_text->EnsureGlyphs(cached_preedit);
                const auto points = typist::DecodeUtf8(cached_preedit);
                const bool covered = std::all_of(points.begin(), points.end(),
                    [&](char32_t c) { return HasGlyph(font, c); });
                if (covered) preedit_font = font;
                else {
                    preedit_font = typist::LoadExerciseFont({cached_preedit}, 120);
                    preedit_owns_font = preedit_font.texture.id != GetFontDefault().texture.id;
                }
            }
        }
        if (!IsMinimal(visuals)) {
            if (!text_input.Preedit().empty() && viewport.Browsing())
                viewport.Return(layout, snapshot.cursor, true);
            viewport.Follow(layout, snapshot.cursor, visuals.ReducedMotion());
            viewport.Update(dt, visuals.ReducedMotion());
            const auto caret = layout.CursorPosition(snapshot.cursor);
            const auto prefix = typist::DecodeUtf8(text_input.Preedit());
            const auto count = std::min<std::size_t>(prefix.size(), std::max(0, text_input.PreeditCursor()));
            const float preedit_x = slug_text->Measure(
                std::span<const char32_t>(prefix.data(), count), exercise_size * 0.55f, 1, preedit_font);
            text_input.SetCandidatePosition(static_cast<int>(exercise_view.x + caret.x + preedit_x),
                static_cast<int>(exercise_view.y + caret.y - viewport.Scroll() + exercise_size * 1.55f),
                static_cast<int>(exercise_size * 0.55f));
        }

        const bool use_bloom = !visuals.ReducedMotion() &&
            (visuals.GetPreset() == typist::Preset::neon ||
             visuals.GetPreset() == typist::Preset::aurora ||
             visuals.GetPreset() == typist::Preset::overdrive);
        auto draw_preedit = [&](Vector2 origin) {
            if (text_input.Preedit().empty() || snapshot.completed) return;
            const auto caret = layout.CursorPosition(snapshot.cursor);
            const float size = exercise_size * 0.55f;
            const Vector2 position{origin.x + caret.x,
                                   origin.y + caret.y - viewport.Scroll() + exercise_size};
            slug_text->Draw(text_input.Preedit(), position, size, visuals.Accent(), 1, preedit_font);
            const auto prefix = typist::DecodeUtf8(text_input.Preedit());
            const auto count = std::min<std::size_t>(prefix.size(), std::max(0, text_input.PreeditCursor()));
            const float cursor_x = position.x + slug_text->Measure(
                std::span<const char32_t>(prefix.data(), count), size, 1, preedit_font);
            DrawRectangle(static_cast<int>(cursor_x), static_cast<int>(position.y + size),
                          3, 3, visuals.Accent());
        };
        BeginDrawing();
        if (!IsMinimal(visuals) && viewport.Begin()) {
            DrawExercise(*slug_text, *session, snapshot, visuals, layout, viewport.Scroll(),
                         viewport.Height(), exercise_size, time, mistake_flash);
            draw_preedit({0, 0});
            viewport.End();
        }
#ifdef TYPIST_SDL3
        // raylib's SDL3 projection uses drawable pixels after a DPI change.
        rlLoadIdentity();
        rlScalef(dpi_x, dpi_y, 1);
#endif
        const bool bloom_active = use_bloom && bloom.Begin(width, height, dpi_x, dpi_y);
        visuals.DrawBackground(width, height, time);
        if (IsMinimal(visuals)) {
            DrawMinimalExercise(*slug_text, *session, snapshot, width, height, mistake_flash);
            EndDrawing();
            if (!screenshot_path.empty() && ++rendered_frames == 3) {
                TakeScreenshot(screenshot_path.c_str());
                break;
            }
            continue;
        }
        if (success_flash > 0 && visuals.GetPreset() != typist::Preset::focus && !visuals.ReducedMotion())
            DrawRectangle(0, 0, width, height, FadeTo(visuals.Accent(), success_flash * 0.08f));

        visuals.DrawExerciseSurface({margin - 26, exercise_top - 34,
                                     width - 2 * margin + 52, exercise_view.height + 34}, time);
        if (viewport.Valid()) DrawTexturePro(viewport.Texture(),
            {0, 0, static_cast<float>(viewport.Width()), -static_cast<float>(viewport.Texture().height)}, exercise_view,
            {0, 0}, 0, WHITE);
        else {
            BeginScissorMode(static_cast<int>(exercise_view.x), static_cast<int>(exercise_view.y),
                             static_cast<int>(exercise_view.width), static_cast<int>(exercise_view.height));
            rlPushMatrix();
            rlTranslatef(exercise_view.x, exercise_view.y, 0);
            DrawExercise(*slug_text, *session, snapshot, visuals, layout, viewport.Scroll(),
                         viewport.Height(), exercise_size, time, mistake_flash);
            rlPopMatrix();
            draw_preedit({exercise_view.x, exercise_view.y});
            EndScissorMode();
        }
        BeginScissorMode(static_cast<int>(exercise_view.x), static_cast<int>(exercise_view.y),
                         static_cast<int>(exercise_view.width), static_cast<int>(exercise_view.height));
        visuals.DrawForeground(width, height, time);
        EndScissorMode();
        if (bloom_active) bloom.EndAndComposite(
            visuals.GetPreset() == typist::Preset::overdrive ? 0.95f :
            visuals.GetPreset() == typist::Preset::neon ? 0.86f : 0.65f);
#ifdef TYPIST_SDL3
        if (bloom_active) {
            rlLoadIdentity();
            rlScalef(dpi_x, dpi_y, 1);
        }
#endif

        const Color muted = visuals.MutedColor();
        const Color foreground = visuals.TextColor();
        Label(*slug_text, titles[lesson_index], margin, height * 0.15f, 32, foreground);
        const char* preset_name = visuals.Name();
        const float name_width = slug_text->Measure(preset_name, 28);
        Label(*slug_text, preset_name, width - margin - name_width, height * 0.16f, 28, visuals.Accent());
        visuals.DrawProgress({margin, progress_y,
                              width - 2 * margin, 6}, static_cast<float>(snapshot.progress));
        const std::string stats = typist::FormatStatus(snapshot);
        Label(*slug_text, stats, margin, std::min(height * 0.76f, height - 150.0f), 30, foreground);
        Label(*slug_text, "F1 STYLE     < > LESSON", margin, height - 108.0f, 26, muted);
        Label(*slug_text, "F5 RETRY     F6 MOTION", margin, height - 66.0f, 26, muted);
        if (viewport.Browsing()) Label(*slug_text, "ESC BACK TO TYPING", width - margin - 320, height - 66, 22, muted);
        if (!notice.empty()) Label(*slug_text, notice, margin, height * 0.25f, 24, RED);
        EndDrawing();
        if (!screenshot_path.empty() && ++rendered_frames == 3) {
            TakeScreenshot(screenshot_path.c_str());
            break;
        }
    }

    save();
    bloom.Shutdown();
    viewport.Shutdown();
    clear_preedit_font();
    slug_text->Shutdown();
    if (font.texture.id != GetFontDefault().texture.id) UnloadFont(font);
    CloseWindow();
}
