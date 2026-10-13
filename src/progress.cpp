#include "progress.h"

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace typist {

namespace {
std::uint64_t Hash(std::string_view text) {
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned char c : text) { hash ^= c; hash *= 1099511628211ull; }
    return hash;
}
}

std::string ProgressPath(std::string_view lesson_id, std::string_view text) {
    const char* state_home = std::getenv("XDG_STATE_HOME");
    const char* home = std::getenv("HOME");
    std::filesystem::path dir = state_home && *state_home ? state_home :
        home && *home ? std::filesystem::path(home) / ".local/state" : ".";
    dir /= "typist101";
    std::ostringstream name;
    name << std::hex << Hash(lesson_id) << '-' << Hash(text) << ".progress";
    return (dir / name.str()).string();
}

bool WriteProgress(const std::string& path, std::string_view text, const SessionState& s) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    if (ec) return false;
    const std::string temp = path + ".tmp";
    {
        std::ofstream file(temp, std::ios::trunc);
        if (!file) return false;
        file << "T101 1 " << std::hex << Hash(text) << std::dec << '\n'
             << s.cursor << ' ' << s.correct_strokes << ' ' << s.mistakes << ' '
             << std::setprecision(17) << s.elapsed_seconds << ' ' << s.started << ' ' << s.completed << '\n';
        if (!file) return false;
    }
    std::filesystem::rename(temp, path, ec);
    return !ec;
}

bool ReadProgress(const std::string& path, std::string_view text, SessionState& state) {
    std::ifstream file(path);
    std::string magic;
    int version = 0;
    std::uint64_t hash = 0;
    SessionState parsed;
    if (!(file >> magic >> version >> std::hex >> hash >> std::dec) ||
        magic != "T101" || version != 1 || hash != Hash(text) ||
        !(file >> parsed.cursor >> parsed.correct_strokes >> parsed.mistakes >>
          parsed.elapsed_seconds >> parsed.started >> parsed.completed)) return false;
    state = parsed;
    return true;
}

}  // namespace typist
