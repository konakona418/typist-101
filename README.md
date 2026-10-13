# typist 101

A typing practice app with large text, saved progress, UTF-8 lessons, and five visual presets. Built with raylib and Slug text rendering.

## Build and run

Requires CMake 3.25+, C++20, SDL3 or SDL2, Fontconfig, libepoxy, and pkg-config. OpenGL 4.3 enables Slug text; older contexts use raylib fonts.

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/typist101
./build/typist101 --lesson lessons/lu-xun-leifeng-simplified.txt
```

On Arch, commit your changes and run `./scripts/install-arch.sh` to build and install the package. Use `./scripts/package-arch.sh` to build it without installing; the package is written to `dist/`. Sample texts are in [lessons](lessons/README.md). Drag any UTF-8 file of up to 1 MiB into the window to load it. Progress lives under `$XDG_STATE_HOME/typist101` or `~/.local/state/typist101`.

## Controls

| Key | Action |
| --- | --- |
| Type / Enter | Match the next character / line break |
| Backspace | Revisit the previous character |
| Left / Right / Tab | Change lesson |
| Mouse wheel / Page Up / Page Down | Browse a long lesson |
| Esc | Return to the current typing line |
| F1 | Cycle Focus, Neon, Aurora, Overdrive, Minimal |
| F5 | Restart the lesson |
| F6 | Reduce motion |

Minimal loops one large line. Long lessons scroll with typing progress. See [TODO.md](TODO.md) for remaining checks.
