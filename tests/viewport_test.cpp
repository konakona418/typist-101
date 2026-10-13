#include "text_layout.h"
#include "text_viewport.h"
#include "session.h"
#include "status.h"
#include "utf8.h"

#include <raylib.h>
#include <epoxy/gl.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

int main() {
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(760, 540, "viewport test");
    std::vector<char32_t> text(240, U'a');
    typist::TextLayout layout;
    layout.Build(text, 10, 20, [](char32_t) { return 10.0f; });
    typist::TextViewport viewport;
    viewport.Configure(100, 120, 1, 1);
    assert(viewport.Valid());
    assert(viewport.Width() == 100);
    assert(viewport.Height() == 120);
    viewport.Follow(layout, 0, true);
    assert(viewport.Scroll() == layout.CenteredScroll(0, 120));
    viewport.Browse(3, layout);
    assert(viewport.Browsing());
    viewport.Update(0.016f, true);
    assert(viewport.Scroll() > layout.CenteredScroll(0, 120));
    const float browsed = viewport.Scroll();
    viewport.Follow(layout, 200, true);
    assert(viewport.Scroll() == browsed);
    viewport.Return(layout, 200, true);
    assert(!viewport.Browsing());
    assert(viewport.Scroll() == layout.CenteredScroll(layout.LineFor(200), 120));

    std::string lesson;
    for (int i = 0; i < 300; ++i) lesson += "Typing 你好。\n";
    const auto lesson_points = typist::DecodeUtf8(lesson);
    typist::TextLayout long_layout;
    long_layout.Build(lesson_points, 100, 40,
                      [](char32_t c) { return c == U'\n' ? 0.0f : 12.0f; }, 30);
    typist::Session long_session(lesson);
    for (std::size_t i = 0; i < lesson_points.size(); ++i) {
        if (i % 137 == 0) {
            viewport.Browse(5, long_layout);
            assert(viewport.Browsing());
            viewport.Return(long_layout, i, true);
        }
        const auto result = long_session.Input(lesson_points[i], 1.0 + i * 0.01);
        assert(result == (i + 1 == lesson_points.size() ? typist::InputResult::completed
                                                      : typist::InputResult::correct));
        viewport.Follow(long_layout, long_session.GetSnapshot(1.0 + i * 0.01).cursor, true);
        const auto [first, last] = long_layout.VisibleLines(viewport.Scroll(), viewport.Height());
        assert(first <= long_layout.LineFor(i) && long_layout.LineFor(i) < last);
        assert(last - first <= 6);
    }
    const auto complete = long_session.GetSnapshot(100);
    assert(complete.completed && complete.progress == 1.0);
    assert(typist::FormatStatus(complete).find("100% DONE") != std::string::npos);
    assert(long_layout.LineFor(complete.cursor) == long_layout.Lines().size() - 1);
    assert(viewport.Scroll() == long_layout.CenteredScroll(long_layout.LineFor(complete.cursor),
                                                          viewport.Height()));
    assert(long_session.Backspace() == typist::InputResult::ignored);

    viewport.Return(long_layout, 0, true);
    viewport.Follow(long_layout, 250, false);
    assert(viewport.Scroll() == viewport.Target()); // Large jumps land immediately.
    const auto next_line = long_layout.LineFor(250) + 1;
    viewport.Follow(long_layout, long_layout.Lines()[next_line].begin, false);
    const float before_animation = viewport.Scroll();
    viewport.Update(0.016f, false);
    assert(viewport.Scroll() > before_animation && viewport.Scroll() < viewport.Target());
    viewport.Update(0.016f, true);
    assert(viewport.Scroll() == viewport.Target());

    viewport.Configure(120, 160, 1, 1);
    assert(viewport.Width() == 120 && viewport.Height() == 160);
    viewport.Return(long_layout, complete.cursor, true);
    assert(viewport.Scroll() == long_layout.CenteredScroll(long_layout.LineFor(complete.cursor), 160));

    viewport.Configure(100, 120, 2, 2);
    assert(viewport.Width() == 200);
    assert(viewport.Texture().height == 240);
    BeginDrawing();
    assert(viewport.Begin());
    DrawRectangle(0, 0, 100, 120, RED);
    viewport.End();
    Image pixels = LoadImageFromTexture(viewport.Texture());
    const Color center = GetImageColor(pixels, 100, 120);
    assert(center.r == RED.r && center.g == RED.g && center.b == RED.b && center.a == 255);
    UnloadImage(pixels);
    ClearBackground(BLACK);
    DrawTexturePro(viewport.Texture(), {0, 0, 200, -240}, {20, 30, 100, 120}, {0, 0}, 0, WHITE);
    EndDrawing();
    unsigned char presented[4]{};
    glReadBuffer(GL_FRONT);
    glReadPixels(70, GetRenderHeight() - 1 - 90, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, presented);
    assert(presented[0] == RED.r && presented[1] == RED.g && presented[2] == RED.b);
    glReadBuffer(GL_BACK);
    Image screen = LoadImageFromScreen();
    const Color outside = GetImageColor(screen, 19, 90);
    const Color inside = GetImageColor(screen, 70, 90);
    assert(outside.r < 20 && outside.g < 20 && outside.b < 20);
    assert(inside.r == RED.r && inside.g == RED.g && inside.b == RED.b);
    UnloadImage(screen);
    viewport.Shutdown();
    CloseWindow();
}
