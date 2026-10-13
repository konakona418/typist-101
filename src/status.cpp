#include "status.h"

#include <cstdio>

namespace typist {

std::string FormatStatus(const Snapshot& snapshot) {
    if (!snapshot.started) return {};
    char text[128];
    if (snapshot.completed) {
        std::snprintf(text, sizeof(text), "100%% DONE   ACC %.0f%%   %.0f WPM",
                      snapshot.accuracy * 100, snapshot.words_per_minute);
    } else {
        std::snprintf(text, sizeof(text), "%.0f%% PROGRESS   ACC %.0f%%   %.0f WPM",
                      snapshot.progress * 100, snapshot.accuracy * 100,
                      snapshot.words_per_minute);
    }
    return text;
}

}  // namespace typist
