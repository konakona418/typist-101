#pragma once

#include "session.h"

#include <string>
#include <string_view>

namespace typist {

std::string ProgressPath(std::string_view lesson_id, std::string_view text);
bool WriteProgress(const std::string& path, std::string_view text, const SessionState& state);
bool ReadProgress(const std::string& path, std::string_view text, SessionState& state);

}  // namespace typist
