#include "lesson.h"
#include "utf8.h"

#include <fstream>
#include <iterator>

namespace typist {

LessonLoad ParseLesson(std::string_view bytes) {
    if (bytes.size() > kMaxLessonBytes) return {{}, "Lesson exceeds 1 MiB"};
    if (bytes.starts_with("\xef\xbb\xbf")) bytes.remove_prefix(3);
    const auto decoded = DecodeUtf8(bytes);
    // DecodeUtf8 replaces malformed bytes; valid UTF-8 round trips unchanged.
    std::string canonical;
    for (char32_t c : decoded) canonical += EncodeUtf8(c);
    if (canonical != bytes) return {{}, "Lesson is not valid UTF-8"};
    std::string text;
    text.reserve(bytes.size());
    bool has_content = false;
    bool previous_cr = false;
    for (char32_t c : decoded) {
        if (c == U'\n' && previous_cr) {
            previous_cr = false;
            continue;
        }
        previous_cr = c == U'\r';
        if (c == U'\r') c = U'\n';
        if (c == U'\t') {
            text += "    ";
            continue;
        }
        if ((c < 0x20 && c != U'\n') || c == 0x7f)
            return {{}, "Lesson contains unsupported control characters"};
        if (c != U' ' && c != U'\n' && c != U'\u3000') has_content = true;
        text += EncodeUtf8(c);
    }
    if (!has_content) return {{}, "Lesson is empty"};
    return {std::move(text), {}};
}

LessonLoad LoadLessonFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {{}, "Cannot open lesson file"};
    std::string bytes(kMaxLessonBytes + 1, '\0');
    file.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    bytes.resize(static_cast<std::size_t>(file.gcount()));
    if (file.bad()) return {{}, "Cannot read lesson file"};
    return ParseLesson(bytes);
}

}  // namespace typist
