# Long text plan

Status: implemented. The active line stays centered with smooth scrolling; reduced motion jumps to it.

## Behavior

- Keep the active line centered, including the first and final lines. Clip text to a viewport sized offscreen target; keep labels and progress fixed on screen.
- Let wheel and Page Up/Down browse without changing progress. Esc, typing, Backspace, or preedit returns to the active line. Completion stays on the final line and shows exactly `100% DONE`.
- Keep Minimal as a separate endless single line. F5 restarts a finite lesson.
- Show explicit LF as an Enter target. Preserve empty lines. Only committed input advances `Session`; preedit and the candidate window follow the visible caret.

## Implementation

1. Parse UTF-8 files (BOM, CRLF/CR, tabs, invalid and empty files, 1 MiB limit). Treat LF as one input codepoint.
2. Cache codepoint positions and line ranges. Wrap Latin words when possible, long words and CJK by character. Rebuild only when lesson, font, size, or width changes.
3. Draw only visible lines into a viewport texture. End its render pass before composing the full scene and bloom. Use the same coordinate transform for Slug, fallback fonts, caret, errors, particles, and IME position.
4. Keep scroll state separate from `Session`. Manual browsing never changes scores. Restore saved cursor and statistics by lesson identity, then locate the current line.
5. Resize the texture with the window and account for high DPI. Provide a clipped direct draw if the texture fails.

## Acceptance

- Type a multi-thousand-character mixed Chinese/English lesson through its final character or LF without losing the caret or stopping short of 100%.
- Scroll away, type, return, Backspace across lines, switch presets, resize, and toggle reduced motion without losing progress or drawing outside the viewport.
- Check preedit, multi-character commits, candidate placement, and cancellation with Fcitx5 and IBus on Wayland and X11.
- Check OpenGL 4.3 Slug and 3.3 fallback, bloom, large text, and high DPI. Per-frame work must scale with visible lines rather than total lesson length.
