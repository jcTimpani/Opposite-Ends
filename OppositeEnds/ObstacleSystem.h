#pragma once

// Pure C++ puzzle rules: no renderer, Windows headers, or game-engine dependencies.
// This makes the obstacle mechanics testable on Windows, macOS, and Linux.
#include <array>
#include <vector>

class ObstacleSystem {
public:
    struct Position {
        int row;
        int col;
    };

    static constexpr int kGridSize = 18;

    ObstacleSystem();

    // One input moves both astronauts in opposite directions.
    // Returns true if at least one astronaut actually moved.
    bool TryMove(int rowDelta, int colDelta);
    bool Undo();
    void Reset();

    bool IsWall(const Position& position) const;
    bool IsCrate(const Position& position) const;

    Position PrimaryPosition() const { return m_primary; }
    Position OppositePosition() const { return m_opposite; }
    int MoveCount() const { return m_moveCount; }

private:
    static constexpr int kCrateCount = 3;
    using Crates = std::array<Position, kCrateCount>;

    struct Snapshot {
        Position primary;
        Position opposite;
        Crates crates;
    };

    Position m_primary{ 4, 4 };
    Position m_opposite{ 13, 13 };
    Crates m_crates{{ { 4, 6 }, { 13, 12 }, { 9, 9 } }};
    std::vector<Snapshot> m_history;
    int m_moveCount = 0;

    int FindCrate(const Position& position) const;
    static bool SamePosition(const Position& a, const Position& b);
};
