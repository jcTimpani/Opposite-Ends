#include "LevelProgress.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>

namespace {
constexpr const char* kHeader = "OPPOSITE_ENDS_SCORES_V1";
constexpr std::uint64_t kOffset = 14695981039346656037ULL;
constexpr std::uint64_t kPrime = 1099511628211ULL;
}

std::uint64_t LevelProgress::FingerprintFile(const std::string& filename) {
    std::ifstream input(filename, std::ios::binary);
    if (!input) return 0;
    std::string bytes((std::istreambuf_iterator<char>(input)),
                      std::istreambuf_iterator<char>());
    if (input.bad() || bytes.empty()) return 0;

    // Git checkouts may use LF or CRLF on different operating systems.
    // They are logically the same level, so hash normalized line endings.
    std::uint64_t hash = kOffset;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(bytes[i]);
        if (ch == '\r') {
            if (i + 1 < bytes.size() && bytes[i + 1] == '\n') ++i;
            ch = '\n';
        }
        hash ^= ch;
        hash *= kPrime;
    }
    return hash == 0 ? 1 : hash; // Zero is reserved for unreadable files.
}

int LevelProgress::StarsFor(int moves, int optimalMoves) {
    if (moves < 0 || optimalMoves < 0 || moves < optimalMoves) return 0;
    if (moves == optimalMoves) return 3;
    // Give a little margin on short levels; harder levels scale up modestly.
    const int allowance = std::max(2, (optimalMoves + 3) / 4);
    return moves <= optimalMoves + allowance ? 2 : 1;
}

int LevelProgress::BestMoves(int level, std::uint64_t fingerprint) const {
    if (level <= 0 || fingerprint == 0) return -1;
    for (const Entry& entry : m_entries)
        if (entry.level == level && entry.fingerprint == fingerprint)
            return entry.bestMoves;
    return -1;
}

bool LevelProgress::RecordCompletion(int level, std::uint64_t fingerprint, int moves) {
    if (level <= 0 || fingerprint == 0 || moves < 0) return false;
    for (Entry& entry : m_entries) {
        if (entry.level == level && entry.fingerprint == fingerprint) {
            if (moves >= entry.bestMoves) return false;
            entry.bestMoves = moves;
            return true;
        }
    }
    m_entries.push_back({ level, fingerprint, moves });
    return true;
}

bool LevelProgress::Load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return false;
    std::string line;
    if (!std::getline(file, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != kHeader) return false;

    LevelProgress parsed;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::istringstream row(line);
        int level = 0, moves = -1;
        std::uint64_t fingerprint = 0;
        std::string trailing;
        if (!(row >> level >> fingerprint >> moves) ||
            (row >> trailing) || level <= 0 || fingerprint == 0 || moves < 0)
            return false;
        parsed.RecordCompletion(level, fingerprint, moves);
        if (parsed.m_entries.size() > 2048) return false;
    }
    if (file.bad()) return false;
    m_entries = std::move(parsed.m_entries);
    return true;
}

bool LevelProgress::Save(const std::string& filename) const {
    const std::string temp = filename + ".tmp";
    const std::string backup = filename + ".bak";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) return false;
        out << kHeader << '\n';
        for (const Entry& entry : m_entries)
            out << entry.level << ' ' << entry.fingerprint << ' ' << entry.bestMoves << '\n';
        out.flush();
        if (!out.good()) return false;
    }
    // Most POSIX systems can replace an existing file with rename. Windows
    // commonly cannot, so preserve a backup before attempting replacement.
    if (std::rename(temp.c_str(), filename.c_str()) == 0) return true;
    std::remove(backup.c_str());
    if (std::rename(filename.c_str(), backup.c_str()) != 0) return false;
    if (std::rename(temp.c_str(), filename.c_str()) == 0) {
        std::remove(backup.c_str());
        return true;
    }
    std::rename(backup.c_str(), filename.c_str());
    return false;
}
