#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace typist {

std::vector<char32_t> DecodeUtf8(std::string_view text);
std::string EncodeUtf8(char32_t codepoint);

}  // namespace typist
