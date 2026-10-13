#include "text_input.h"

#ifdef TYPIST_SDL_IME
#ifdef TYPIST_SDL3
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif
#endif

#include <utility>

namespace typist {

#ifdef TYPIST_SDL_IME
namespace {
#ifdef TYPIST_SDL3
bool WatchSDL(void* data, SDL_Event* event) { return TextInput::Watch(data, event) == 0; }
#else
int WatchSDL(void* data, SDL_Event* event) { return TextInput::Watch(data, event); }
#endif
}
#endif

TextInput::TextInput(void* window) : window_(window) {
#ifdef TYPIST_SDL_IME
    SDL_AddEventWatch(WatchSDL, this);
#ifdef TYPIST_SDL3
    if (!window_) window_ = SDL_GetKeyboardFocus();
    if (window_) SDL_StartTextInput(static_cast<SDL_Window*>(window_));
#else
    (void)window;
    SDL_StartTextInput();
#endif
#endif
}

TextInput::~TextInput() {
#ifdef TYPIST_SDL_IME
#ifdef TYPIST_SDL3
    SDL_RemoveEventWatch(WatchSDL, this);
#else
    SDL_DelEventWatch(WatchSDL, this);
#endif
#ifdef TYPIST_SDL3
    if (window_) SDL_StopTextInput(static_cast<SDL_Window*>(window_));
#else
    SDL_StopTextInput();
#endif
#endif
}

int TextInput::Watch(void* data, void* raw_event) {
#ifdef TYPIST_SDL_IME
    auto& self = *static_cast<TextInput*>(data);
    const auto& event = *static_cast<SDL_Event*>(raw_event);
#ifdef TYPIST_SDL3
    if (event.type == SDL_EVENT_TEXT_INPUT) {
#else
    if (event.type == SDL_TEXTINPUT) {
#endif
        if (!self.preedit_.empty()) self.composition_changed_ = true;
        self.committed_.emplace_back(event.text.text ? event.text.text : "");
        self.preedit_.clear();
        self.preedit_cursor_ = 0;
    }
#ifdef TYPIST_SDL3
    else if (event.type == SDL_EVENT_TEXT_EDITING) {
#else
    else if (event.type == SDL_TEXTEDITING) {
#endif
        self.composition_changed_ = true;
        self.preedit_ = event.edit.text ? event.edit.text : "";
        self.preedit_cursor_ = event.edit.start;
    }
#else
    (void)data; (void)raw_event;
#endif
    return 0;
}

std::vector<std::string> TextInput::TakeCommitted() {
    return std::exchange(committed_, {});
}

bool TextInput::ConsumeCompositionActivity() {
    const bool active = composition_changed_ || !preedit_.empty();
    composition_changed_ = false;
    return active;
}

void TextInput::SetCandidatePosition(int x, int y, int height) {
#ifdef TYPIST_SDL_IME
    SDL_Rect rect{x, y, 2, height};
#ifdef TYPIST_SDL3
    if (!window_) window_ = SDL_GetKeyboardFocus();
    if (window_) SDL_SetTextInputArea(static_cast<SDL_Window*>(window_), &rect, 0);
#else
    SDL_SetTextInputRect(&rect);
#endif
#else
    (void)x; (void)y; (void)height;
#endif
}

}  // namespace typist
