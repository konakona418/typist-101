#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace typist {

struct LessonLoad {
    std::string text;
    std::string error;
    explicit operator bool() const { return error.empty(); }
};

constexpr std::size_t kMaxLessonBytes = 1024 * 1024;
LessonLoad ParseLesson(std::string_view bytes);
LessonLoad LoadLessonFile(const std::string& path);

}  // namespace typist
