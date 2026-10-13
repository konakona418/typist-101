#pragma once

#include <string>
#include <vector>

namespace typist {

class TextInput {
public:
    explicit TextInput(void* window = nullptr);
    ~TextInput();
    std::vector<std::string> TakeCommitted();
    const std::string& Preedit() const { return preedit_; }
    int PreeditCursor() const { return preedit_cursor_; }
    bool ConsumeCompositionActivity();
    void SetCandidatePosition(int x, int y, int height);
    static int Watch(void* data, void* event);

private:
    std::vector<std::string> committed_;
    std::string preedit_;
    int preedit_cursor_ = 0;
    bool composition_changed_ = false;
    void* window_ = nullptr;
};

}  // namespace typist
