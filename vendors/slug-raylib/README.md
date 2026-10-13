# Slug integration

`slug.h` is adapted from `/home/lain/Documents/Repositories/raylyrics/vendors/slug-raylib/slug.h`, based on the Slug implementation by Zimeng Li and Eric Lengyel's GPU font rendering research. `stb_truetype.h` comes from raylyrics' vendored raylib; its MIT/Public Domain license text is included at the end of that file.

Local changes to `slug.h`:

- Use libepoxy OpenGL functions for SSBO creation, updates and binding. The system raylib is compiled for OpenGL 3.3, where raylib's own SSBO functions are stubs.
- Flush raylib's draw batch before direct GL drawing and bind the Slug program explicitly.
- Keep the upstream Slug outline and coverage shaders unchanged.

The application detects OpenGL 4.3 before loading Slug and falls back to raylib's font atlas if unavailable.
