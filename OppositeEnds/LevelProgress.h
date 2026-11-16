#pragma once

// Portable move-based level ratings and best-score persistence.
// Does not depend on the Windows renderer or game engine.
#include <cstdint>
#include <string>
#include <vector>

class LevelProgress {
public:
    // A changed level has a different fingerprint and cannot inherit an old best.
    static std::uint64_t FingerprintFile(const std::string& filename);
    static int StarsFor(int moves, int optimalMoves);

    // Returns true only if this completion improves the personal best.
    bool RecordCompletion(int level, std::uint64_t fingerprint, int moves);
    // Returns -1 when this level version has no saved completion.
    int BestMoves(int level, std::uint64_t fingerprint) const;

    // Load accepts only the versioned format created by Save. Missing or invalid
    // files leave existing progress untouched and return false.
    bool Load(const std::string& filename);
    bool Save(const std::string& filename) const;

private:
    struct Entry {
        int level;
        std::uint64_t fingerprint;
        int bestMoves;
    };
    std::vector<Entry> m_entries;
};
