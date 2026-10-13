#include "utf8.h"

namespace typist {

std::vector<char32_t> DecodeUtf8(std::string_view text) {
    std::vector<char32_t> result;
    for (std::size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text[i]);
        if (first < 0x80) {
            result.push_back(first);
            ++i;
            continue;
        }

        int length = 0;
        char32_t value = 0;
        char32_t minimum = 0;
        if ((first & 0xe0) == 0xc0) {
            length = 2; value = first & 0x1f; minimum = 0x80;
        } else if ((first & 0xf0) == 0xe0) {
            length = 3; value = first & 0x0f; minimum = 0x800;
        } else if ((first & 0xf8) == 0xf0) {
            length = 4; value = first & 0x07; minimum = 0x10000;
        }
        bool valid = length != 0 && i + length <= text.size();
        if (valid) {
            for (int j = 1; j < length; ++j) {
                const auto byte = static_cast<unsigned char>(text[i + j]);
                if ((byte & 0xc0) != 0x80) { valid = false; break; }
                value = (value << 6) | (byte & 0x3f);
            }
            valid = valid && value >= minimum && value <= 0x10ffff &&
                    !(value >= 0xd800 && value <= 0xdfff);
        }
        result.push_back(valid ? value : U'\ufffd');
        i += valid ? length : 1;
    }
    return result;
}

std::string EncodeUtf8(char32_t c) {
    if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) c = U'\ufffd';
    std::string result;
    if (c < 0x80) result.push_back(static_cast<char>(c));
    else if (c < 0x800) {
        result.push_back(static_cast<char>(0xc0 | (c >> 6)));
        result.push_back(static_cast<char>(0x80 | (c & 0x3f)));
    } else if (c < 0x10000) {
        result.push_back(static_cast<char>(0xe0 | (c >> 12)));
        result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | (c & 0x3f)));
    } else {
        result.push_back(static_cast<char>(0xf0 | (c >> 18)));
        result.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | (c & 0x3f)));
    }
    return result;
}

}  // namespace typist
