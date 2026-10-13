# Design notes

The app separates committed text, lesson state, layout, and rendering:

```text
SDL text input -> Session -> Snapshot -> text viewport -> raylib/Slug
lesson file ------^                         -> visual preset
```

`Session` advances by Unicode codepoint. Preedit text is displayed but never scored. A committed SDL text chunk may contain several codepoints. The candidate rectangle follows the visible caret. The SDL backend owns the window and input events.

Finite lessons save their cursor, attempts, mistakes, and elapsed time. Minimal is an endless loop and does not save finite progress. Layout is rebuilt on lesson, font, size, or width changes; typing only redraws visible lines.

Slug draws all text on OpenGL 4.3+; raylib fonts provide fallback. Bloom is applied after the exercise viewport is composited into the scene. Reduced motion disables animation and bloom.
