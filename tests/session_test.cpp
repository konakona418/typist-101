#include "session.h"
#include "status.h"
#include "lesson.h"
#include "progress.h"
#include "text_layout.h"
#include "utf8.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>

int main() {
    using namespace typist;
    assert(DecodeUtf8("你a").size() == 2);
    assert(EncodeUtf8(U'你') == "你");
    Session s("你a");
    assert(s.Input(U'错', 10) == InputResult::mistake);
    assert(s.GetSnapshot(10).cursor == 0);
    assert(s.Input(U'你', 11) == InputResult::correct);
    assert(s.GetSnapshot(11).progress == 0.5);
    assert(s.Backspace() == InputResult::erased);
    assert(s.Input(U'你', 12) == InputResult::correct);
    assert(s.Input(U'a', 13) == InputResult::completed);
    const auto result = s.GetSnapshot(100);
    assert(result.completed && result.cursor == 2 && result.mistakes == 1);
    assert(std::abs(result.elapsed_seconds - 3.0) < 0.001);
    assert(std::abs(result.accuracy - 0.75) < 0.001);
    assert(result.correct_strokes == 3); // Retyped character counts again.
    assert(result.cursor == result.total);
    assert(result.progress == 1.0);
    assert(FormatStatus(result).find("100% DONE") != std::string::npos);
    assert(FormatStatus(result).find("ACC 75%") != std::string::npos);
    assert(s.Input(U'b', 101) == InputResult::ignored);

    Session warmup("a");
    assert(warmup.Input(U'a', 1.001) == InputResult::completed);
    assert(warmup.GetSnapshot(1.002).words_per_minute == 0);

    Session errors("ab");
    assert(errors.Input(U'x', 1) == InputResult::mistake);
    assert(errors.Input(U'y', 2) == InputResult::mistake);
    assert(errors.Input(U'a', 3) == InputResult::correct);
    assert(errors.GetSnapshot(61).accuracy == 1.0 / 3.0);
    assert(std::abs(errors.GetSnapshot(61).words_per_minute - 0.2) < 0.001);
    assert(errors.Backspace() == InputResult::erased);
    assert(errors.GetSnapshot(61).words_per_minute == 0);
    assert(errors.Input(U'a', 62) == InputResult::correct);
    assert(errors.Input(U'b', 63) == InputResult::completed);
    assert(errors.GetSnapshot(100).progress == 1.0);
    assert(errors.GetSnapshot(100).accuracy == 0.6);

    Session loop("ab", true);
    assert(loop.Input(U'a', 1) == InputResult::correct);
    assert(loop.Input(U'b', 2) == InputResult::correct);
    assert(loop.Input(U'a', 3) == InputResult::correct);
    assert(loop.GetSnapshot(3).cursor == 3);
    assert(loop.GetSnapshot(3).total == 0);
    assert(loop.GetSnapshot(3).progress == 0.0);
    assert(!loop.GetSnapshot(3).completed);
    assert(loop.Backspace() == InputResult::erased);
    assert(loop.Input(U'a', 4) == InputResult::correct);

    const auto lesson = ParseLesson("\xef\xbb\xbf你\r\n好\r\n\r\nend\t!");
    assert(lesson && lesson.text == "你\n好\n\nend    !");
    assert(!ParseLesson("\xff"));
    assert(!ParseLesson("  \n  "));
    assert(!ParseLesson("a\x01"));
    assert(ParseLesson(std::string(kMaxLessonBytes, 'a')));
    assert(!ParseLesson(std::string(kMaxLessonBytes + 1, 'a')));
    const auto lesson_path = (std::filesystem::temp_directory_path() / "typist101-lesson-test.txt").string();
    {
        std::ofstream lesson_file(lesson_path, std::ios::binary);
        lesson_file << "\xef\xbb\xbf" << "你\r\n好";
    }
    const auto file_lesson = LoadLessonFile(lesson_path);
    assert(file_lesson && file_lesson.text == "你\n好");
    std::filesystem::remove(lesson_path);
    assert(!LoadLessonFile(lesson_path));
    Session paragraphs("a\n\nb\n");
    for (char32_t c : std::vector<char32_t>{U'a', U'\n', U'\n', U'b'})
        assert(paragraphs.Input(c, 10) == InputResult::correct);
    assert(paragraphs.Input(U'\n', 11) == InputResult::completed);
    assert(paragraphs.GetSnapshot(11).progress == 1.0);

    TextLayout layout;
    const auto points = DecodeUtf8("abc def\n你我");
    layout.Build(points, 4, 2, [](char32_t) { return 1.0f; });
    assert(layout.Lines().size() == 3);
    assert(layout.CursorPosition(4).y == 2);
    assert(layout.CursorPosition(points.size()).y == 4);
    assert(layout.LineFor(points.size()) == 2);
    assert(layout.VisibleLines(1, 2).first == 0);
    assert(layout.CenteredScroll(0, 6) == -2);
    assert(layout.CenteredScroll(2, 6) == 2);

    TextLayout newline_layout;
    const auto newline_points = DecodeUtf8("abc\ndef");
    newline_layout.Build(newline_points, 4, 2, [](char32_t) { return 1.0f; }, 2);
    assert(newline_layout.Lines().size() == 3);
    assert(newline_layout.LineFor(3) == 1);
    assert(newline_layout.CursorPosition(3).x == 0);
    assert(newline_layout.Lines()[1].begin == 3 && newline_layout.Lines()[1].end == 3);

    Session saved("hello\nworld");
    assert(saved.Input(U'h', 20) == InputResult::correct);
    assert(saved.Input(U'x', 21) == InputResult::mistake);
    assert(saved.Input(U'e', 22) == InputResult::correct);
    const auto state = saved.Save(23);
    const auto path = (std::filesystem::temp_directory_path() / "typist101-test.progress").string();
    assert(WriteProgress(path, "hello\nworld", state));
    SessionState restored_state;
    assert(ReadProgress(path, "hello\nworld", restored_state));
    assert(!ReadProgress(path, "changed", restored_state));
    Session resumed("hello\nworld");
    assert(resumed.Restore(restored_state, 100));
    const auto resumed_snapshot = resumed.GetSnapshot(101);
    assert(resumed_snapshot.cursor == 2 && resumed_snapshot.mistakes == 1);
    assert(std::abs(resumed_snapshot.elapsed_seconds - 4) < 0.001);
    std::filesystem::remove(path);

    // Repeated lesson switches after a fresh launch: restored start time may
    // legitimately precede this process's monotonic clock origin.
    const auto directory = std::filesystem::temp_directory_path() / "typist101-switch-test";
    std::filesystem::create_directories(directory);
    const std::string texts[] = {"abcdef", "123456"};
    const std::string paths[] = {(directory / "first.progress").string(),
                                 (directory / "second.progress").string()};
    Session first(texts[0]);
    assert(first.Input(U'a', 1) == InputResult::correct);
    assert(first.Input(U'b', 2) == InputResult::correct);
    assert(WriteProgress(paths[0], texts[0], first.Save(121)));
    Session second(texts[1]);
    assert(second.Input(U'1', 4) == InputResult::correct);
    assert(WriteProgress(paths[1], texts[1], second.Save(64)));
    for (int switch_number = 0; switch_number < 6; ++switch_number) {
        const int index = switch_number % 2;
        SessionState persisted;
        assert(ReadProgress(paths[index], texts[index], persisted));
        Session active(texts[index]);
        const double now = 0.25 + switch_number;
        assert(active.Restore(persisted, now));
        const auto snapshot = active.GetSnapshot(now);
        assert(snapshot.started);
        assert(snapshot.cursor == (index == 0 ? 2u : 1u));
        assert(snapshot.elapsed_seconds >= 60);
        assert(WriteProgress(paths[index], texts[index], active.Save(now)));
    }
    std::filesystem::remove_all(directory);

    std::string long_text;
    for (int i = 0; i < 80; ++i) long_text += "steady pace 你好。\n";
    const auto long_points = DecodeUtf8(long_text);
    TextLayout long_layout;
    long_layout.Build(long_points, 12, 2, [](char32_t c) { return c == U'\n' ? 0.0f : 1.0f; });
    assert(long_layout.Lines().size() > 80);
    for (std::size_t i = 0; i < long_points.size(); ++i) {
        const auto line = long_layout.LineFor(i);
        const auto scroll = long_layout.CenteredScroll(line, 6);
        const auto [visible_begin, visible_end] = long_layout.VisibleLines(scroll, 6);
        assert(visible_begin <= line && line < visible_end);
        assert(visible_end - visible_begin <= 6);
    }
    Session long_session(long_text);
    for (std::size_t i = 0; i < long_points.size(); ++i) {
        const auto actual = long_session.Input(long_points[i], 200 + i * 0.01);
        assert(actual == (i + 1 == long_points.size() ? InputResult::completed : InputResult::correct));
    }
    const auto finished = long_session.GetSnapshot(999);
    assert(finished.completed && finished.progress == 1.0);
    assert(finished.cursor == long_layout.Size());
    assert(long_layout.LineFor(finished.cursor) == long_layout.Lines().size() - 1);
}
