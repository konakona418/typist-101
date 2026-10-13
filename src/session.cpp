#include "session.h"
#include "utf8.h"

#include <algorithm>
#include <cmath>

namespace typist {

Session::Session(std::string text, bool looping) : text_(DecodeUtf8(text)), looping_(looping) {}

InputResult Session::Input(char32_t codepoint, double now_seconds) {
    if (text_.empty() || (!looping_ && cursor_ >= text_.size()) ||
        (codepoint < 0x20 && codepoint != U'\n'))
        return InputResult::ignored;
    if (!started_) { started_at_ = now_seconds; started_ = true; }
    if (codepoint != text_[cursor_ % text_.size()]) {
        ++mistakes_;
        return InputResult::mistake;
    }
    ++cursor_;
    ++correct_strokes_;
    if (!looping_ && cursor_ == text_.size()) {
        finished_at_ = now_seconds;
        finished_ = true;
        return InputResult::completed;
    }
    return InputResult::correct;
}

InputResult Session::Backspace() {
    if (cursor_ == 0 || finished_) return InputResult::ignored;
    --cursor_;
    return InputResult::erased;
}

Snapshot Session::GetSnapshot(double now_seconds) const {
    Snapshot s;
    s.cursor = cursor_;
    s.total = looping_ ? 0 : text_.size();
    s.correct_strokes = correct_strokes_;
    s.mistakes = mistakes_;
    s.started = started_;
    s.completed = !looping_ && (finished_ || text_.empty());
    s.progress = s.total ? static_cast<double>(cursor_) / static_cast<double>(s.total)
                         : (s.completed ? 1.0 : 0.0);
    if (s.started) s.elapsed_seconds = std::max(0.0, (s.completed ? finished_at_ : now_seconds) - started_at_);
    const auto strokes = correct_strokes_ + mistakes_;
    if (strokes) s.accuracy = static_cast<double>(correct_strokes_) / static_cast<double>(strokes);
    // The first few milliseconds otherwise produce meaningless six-digit WPM.
    if (s.elapsed_seconds >= 1.0)
        s.words_per_minute = (static_cast<double>(cursor_) / 5.0) * 60.0 / s.elapsed_seconds;
    return s;
}

SessionState Session::Save(double now_seconds) const {
    const auto s = GetSnapshot(now_seconds);
    return {s.cursor, s.correct_strokes, s.mistakes, s.elapsed_seconds, s.started, s.completed};
}

bool Session::Restore(const SessionState& s, double now_seconds) {
    if (looping_ || s.cursor > text_.size() || s.correct_strokes < s.cursor ||
        !std::isfinite(s.elapsed_seconds) || s.elapsed_seconds < 0 ||
        (s.completed != (s.cursor == text_.size())) || (!s.started && s.cursor != 0))
        return false;
    cursor_ = s.cursor;
    correct_strokes_ = s.correct_strokes;
    mistakes_ = s.mistakes;
    started_at_ = s.started ? now_seconds - s.elapsed_seconds : -1;
    finished_at_ = s.completed ? now_seconds : -1;
    started_ = s.started;
    finished_ = s.completed;
    return true;
}

}  // namespace typist
