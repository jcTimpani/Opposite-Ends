#pragma once

// Engine-independent board, level parsing, movement and undo logic.
#include <cstddef>
#include <string>
#include <vector>

class ObstacleSystem {
public:
    struct Position {
        int row;
        int col;
    };

    static constexpr int kGridSize = 18; // Maximum dimensions at 40 pixels/tile.

    ObstacleSystem() = default;

    // Parses a rectangular text map; on failure, keeps the previous level intact.
    bool LoadFromFile(const std::string& path, std::string* error = nullptr);
    bool LoadFromText(const std::string& text, std::string* error = nullptr);

    bool TryMove(int rowDelta, int colDelta);
    bool Undo();
    void Reset();

    bool IsWall(const Position& position) const;
    bool IsCrate(const Position& position) const;
    bool IsPlate(const Position& position) const;
    bool IsPlateOccupied(const Position& position) const;
    bool IsDoor(const Position& position) const;
    bool HasDoor() const { return !m_doors.empty(); }
    bool IsPlatePressed() const;
    bool IsDoorOpen() const;
    bool IsWon() const;

    Position PrimaryPosition() const { return m_primary; }
    Position OppositePosition() const { return m_opposite; }
    Position PrimaryGoal() const { return m_primaryGoal; }
    Position OppositeGoal() const { return m_oppositeGoal; }
    int Width() const { return m_width; }
    int Height() const { return m_height; }
    int MoveCount() const { return m_moveCount; }
    std::size_t CrateCount() const { return m_crates.size(); }

private:
    using Crates = std::vector<Position>;
    struct Snapshot {
        Position primary;
        Position opposite;
        Crates crates;
    };

    int m_width = 0;
    int m_height = 0;
    std::vector<std::string> m_tiles;
    std::vector<Position> m_plates;
    std::vector<Position> m_doors;
    Position m_primaryStart{ 0, 0 };
    Position m_oppositeStart{ 0, 0 };
    Position m_primaryGoal{ 0, 0 };
    Position m_oppositeGoal{ 0, 0 };
    Crates m_initialCrates;

    Position m_primary{ 0, 0 };
    Position m_opposite{ 0, 0 };
    Crates m_crates;
    std::vector<Snapshot> m_history;
    int m_moveCount = 0;

    bool InBounds(const Position& position) const;
    bool IsBlocked(const Position& position) const;
    bool IsDoorOccupied() const;
    int FindCrate(const Position& position) const;
    static bool SamePosition(const Position& a, const Position& b);
};
