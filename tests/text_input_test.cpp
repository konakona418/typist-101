#include "text_input.h"
#include "session.h"
#include "utf8.h"

#ifdef TYPIST_SDL3
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdlib>

int main() {
    setenv("SDL_VIDEODRIVER", "offscreen", 1);
#ifdef TYPIST_SDL3
    assert(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window* window = SDL_CreateWindow("input test", 320, 200, 0);
    assert(window != nullptr);
#else
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
#endif
    {
#ifdef TYPIST_SDL3
        typist::TextInput input(window);
#else
        typist::TextInput input;
#endif
        typist::Session session("你好呀");
        auto dispatch = [&](SDL_Event& event) {
#ifdef TYPIST_SDL3
            SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
            assert(SDL_PushEvent(&event));
            SDL_Event received{};
            assert(SDL_PollEvent(&received));
            assert(received.type == event.type);
#else
            typist::TextInput::Watch(&input, &event);
#endif
        };
        SDL_Event edit{};
        edit.type =
#ifdef TYPIST_SDL3
            SDL_EVENT_TEXT_EDITING;
        edit.edit.text = "nihao";
#else
            SDL_TEXTEDITING;
        std::strcpy(edit.edit.text, "nihao");
#endif
        edit.edit.start = 2;
        dispatch(edit);
        assert(input.Preedit() == "nihao");
        assert(input.PreeditCursor() == 2);
        assert(input.ConsumeCompositionActivity());
        assert(input.ConsumeCompositionActivity());
        assert(session.GetSnapshot(1).cursor == 0);
        input.SetCandidatePosition(30, 40, 20);
#ifdef TYPIST_SDL3
        SDL_Rect candidate_area{};
        int candidate_cursor = -1;
        assert(SDL_GetTextInputArea(window, &candidate_area, &candidate_cursor));
        assert(candidate_area.x == 30 && candidate_area.y == 40);
        assert(candidate_area.w == 2 && candidate_area.h == 20 && candidate_cursor == 0);
#endif

        SDL_Event cancelled{};
        cancelled.type =
#ifdef TYPIST_SDL3
            SDL_EVENT_TEXT_EDITING;
        cancelled.edit.text = "";
#else
            SDL_TEXTEDITING;
        cancelled.edit.text[0] = '\0';
#endif
        dispatch(cancelled);
        assert(input.Preedit().empty());
        assert(input.ConsumeCompositionActivity());
        assert(!input.ConsumeCompositionActivity());
        assert(session.GetSnapshot(2).cursor == 0);

        dispatch(edit);

        SDL_Event commit{};
        commit.type =
#ifdef TYPIST_SDL3
            SDL_EVENT_TEXT_INPUT;
        commit.text.text = "你好";
#else
            SDL_TEXTINPUT;
        std::strcpy(commit.text.text, "你好");
#endif
        dispatch(commit);
        const auto chunks = input.TakeCommitted();
        assert(chunks.size() == 1 && chunks[0] == "你好");
        for (char32_t c : typist::DecodeUtf8(chunks[0])) session.Input(c, 3);
        assert(session.GetSnapshot(3).cursor == 2);
        assert(session.Backspace() == typist::InputResult::erased);
        assert(session.GetSnapshot(3).cursor == 1);
        assert(input.Preedit().empty());
        assert(input.ConsumeCompositionActivity());
        assert(input.TakeCommitted().empty());

#ifdef TYPIST_SDL3
        commit.text.text = "好呀";
#else
        std::strcpy(commit.text.text, "好呀");
#endif
        dispatch(commit);
        for (const auto& chunk : input.TakeCommitted())
            for (char32_t c : typist::DecodeUtf8(chunk)) session.Input(c, 4);
        const auto finished = session.GetSnapshot(4);
        assert(finished.completed && finished.progress == 1.0);
    }
#ifdef TYPIST_SDL3
    SDL_DestroyWindow(window);
#endif
    SDL_Quit();
}
