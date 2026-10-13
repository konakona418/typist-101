#include "bloom.h"

#include <raylib.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main() {
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(200, 120, "bloom test");
    typist::Bloom bloom;
    for (float dpi : {1.0f, 2.0f}) {
        BeginDrawing();
        assert(bloom.Begin(static_cast<int>(200 / dpi), static_cast<int>(120 / dpi), dpi, dpi));
        ClearBackground(BLACK);
        DrawRectangle(10, 10, 20, 20, WHITE);
        bloom.EndAndComposite(0.8f);
        EndDrawing();
        Image pixels = LoadImageFromScreen();
        const Color center = GetImageColor(pixels, static_cast<int>(20 * dpi), static_cast<int>(20 * dpi));
        const Color halo = GetImageColor(pixels, static_cast<int>(34 * dpi), static_cast<int>(20 * dpi));
        const Color corner = GetImageColor(pixels, 180, 100);
        assert(center.r > 200 && center.g > 200 && center.b > 200);
        assert(halo.r > 8 && halo.g > 8 && halo.b > 8);
        assert(corner.r < 20 && corner.g < 20 && corner.b < 20);
        UnloadImage(pixels);
    }
    bloom.Shutdown();
    CloseWindow();
}
