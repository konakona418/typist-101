#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace typist {

enum class InputResult { ignored, correct, mistake, completed, erased };

struct Snapshot {
    std::size_t cursor = 0;
    std::size_t total = 0;
    // Every accepted character is an attempt. Retyping after Backspace is a
    // new attempt; Backspace itself is not one.
    std::size_t correct_strokes = 0;
    std::size_t mistakes = 0;
    double elapsed_seconds = 0;
    double progress = 0;
    double accuracy = 1;
    // Net progress: cursor / 5 / active minutes; freezes on completion.
    double words_per_minute = 0;
    bool started = false;
    bool completed = false;
};

struct SessionState {
    std::size_t cursor = 0;
    std::size_t correct_strokes = 0;
    std::size_t mistakes = 0;
    double elapsed_seconds = 0;
    bool started = false;
    bool completed = false;
};

class Session {
public:
    explicit Session(std::string text, bool looping = false);
    InputResult Input(char32_t codepoint, double now_seconds);
    InputResult Backspace();
    Snapshot GetSnapshot(double now_seconds) const;
    SessionState Save(double now_seconds) const;
    bool Restore(const SessionState& state, double now_seconds);
    const std::vector<char32_t>& Text() const { return text_; }

private:
    std::vector<char32_t> text_;
    bool looping_ = false;
    std::size_t cursor_ = 0;
    std::size_t correct_strokes_ = 0;
    std::size_t mistakes_ = 0;
    double started_at_ = -1;
    double finished_at_ = -1;
    bool started_ = false;
    bool finished_ = false;
};

}  // namespace typist
