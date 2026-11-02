#include "ObstacleSystem.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>

namespace {
    ObstacleSystem::Position Advance(const ObstacleSystem::Position& p, int dr, int dc) {
        return { p.row + dr, p.col + dc };
    }

    bool Fail(std::string* error, const std::string& message) {
        if (error) *error = message;
        return false;
    }
}

bool ObstacleSystem::LoadFromFile(const std::string& path, std::string* error) {
    std::ifstream file(path);
    if (!file) return Fail(error, "Cannot open " + path);
    std::ostringstream contents;
    contents << file.rdbuf();
    if (!file.good() && !file.eof()) return Fail(error, "Cannot read " + path);
    return LoadFromText(contents.str(), error);
}

bool ObstacleSystem::LoadFromText(const std::string& text, std::string* error) {
    std::istringstream input(text);
    std::vector<std::string> tiles;
    std::string line;
    while (std::getline(input, line)) {
        // Windows CRLF files work on other platforms, too.
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) return Fail(error, "Blank rows are not allowed");
        if (line.size() > static_cast<std::size_t>(kGridSize))
            return Fail(error, "Level is wider than 18 tiles");
        if (!tiles.empty() && line.size() != tiles[0].size())
            return Fail(error, "Every row must have the same width");
        tiles.push_back(line);
        if (tiles.size() > static_cast<std::size_t>(kGridSize))
            return Fail(error, "Level is taller than 18 tiles");
    }
    if (tiles.empty() || tiles[0].empty()) return Fail(error, "Level is empty");

    Position primary{ 0, 0 }, opposite{ 0, 0 }, primaryGoal{ 0, 0 }, oppositeGoal{ 0, 0 };
    int redCount = 0, blueCount = 0, redGoalCount = 0, blueGoalCount = 0;
    std::vector<Position> plates, doors, crates;

    for (int row = 0; row < static_cast<int>(tiles.size()); ++row) {
        for (int col = 0; col < static_cast<int>(tiles[row].size()); ++col) {
            const Position pos{ row, col };
            switch (tiles[row][col]) {
            case 'e': case 'w': break;
            case 'r': primary = pos; ++redCount; break;
            case 'b': opposite = pos; ++blueCount; break;
            case 'c': primaryGoal = pos; ++redGoalCount; break;
            case 'h': oppositeGoal = pos; ++blueGoalCount; break;
            case 'p': plates.push_back(pos); break;
            case 'd': doors.push_back(pos); break;
            case 'k': crates.push_back(pos); break;
            default:
                return Fail(error, "Unknown tile at row " + std::to_string(row + 1) +
                    ", column " + std::to_string(col + 1));
            }
        }
    }
    if (redCount != 1 || blueCount != 1 || redGoalCount != 1 || blueGoalCount != 1)
        return Fail(error, "Need exactly one r, b, c, and h in each level");
    if (!doors.empty() && plates.empty())
        return Fail(error, "A door needs at least one pressure plate");

    // Commit atomically only after the map has been validated.
    m_tiles = std::move(tiles);
    m_width = static_cast<int>(m_tiles[0].size());
    m_height = static_cast<int>(m_tiles.size());
    m_primaryStart = primary;
    m_oppositeStart = opposite;
    m_primaryGoal = primaryGoal;
    m_oppositeGoal = oppositeGoal;
    m_plates = std::move(plates);
    m_doors = std::move(doors);
    m_initialCrates = std::move(crates);
    Reset();
    if (error) error->clear();
    return true;
}

bool ObstacleSystem::SamePosition(const Position& a, const Position& b) {
    return a.row == b.row && a.col == b.col;
}

bool ObstacleSystem::InBounds(const Position& pos) const {
    return pos.row >= 0 && pos.col >= 0 && pos.row < m_height && pos.col < m_width;
}

bool ObstacleSystem::IsWall(const Position& pos) const {
    // Out-of-bounds is solid, so maps don't rely on a decorative border.
    return !InBounds(pos) || m_tiles[pos.row][pos.col] == 'w';
}

bool ObstacleSystem::IsPlate(const Position& pos) const {
    return InBounds(pos) && m_tiles[pos.row][pos.col] == 'p';
}

bool ObstacleSystem::IsDoor(const Position& pos) const {
    return InBounds(pos) && m_tiles[pos.row][pos.col] == 'd';
}

bool ObstacleSystem::IsPlateOccupied(const Position& pos) const {
    return IsPlate(pos) && (SamePosition(pos, m_primary) ||
        SamePosition(pos, m_opposite) || IsCrate(pos));
}

bool ObstacleSystem::IsPlatePressed() const {
    for (const auto& plate : m_plates) {
        if (IsPlateOccupied(plate)) return true;
    }
    return false;
}

bool ObstacleSystem::IsDoorOccupied() const {
    for (const auto& door : m_doors) {
        if (SamePosition(door, m_primary) || SamePosition(door, m_opposite) || IsCrate(door))
            return true;
    }
    return false;
}

bool ObstacleSystem::IsDoorOpen() const {
    return IsPlatePressed() || IsDoorOccupied();
}

bool ObstacleSystem::IsBlocked(const Position& pos) const {
    return IsWall(pos) || (IsDoor(pos) && !IsDoorOpen());
}

int ObstacleSystem::FindCrate(const Position& pos) const {
    for (std::size_t i = 0; i < m_crates.size(); ++i) {
        if (SamePosition(pos, m_crates[i])) return static_cast<int>(i);
    }
    return -1;
}

bool ObstacleSystem::IsCrate(const Position& pos) const { return FindCrate(pos) >= 0; }

bool ObstacleSystem::IsWon() const {
    return m_width > 0 && SamePosition(m_primary, m_primaryGoal) &&
        SamePosition(m_opposite, m_oppositeGoal);
}

bool ObstacleSystem::TryMove(int rowDelta, int colDelta) {
    if (m_width == 0 || std::abs(rowDelta) + std::abs(colDelta) != 1) return false;

    struct MovePlan {
        bool allowed = false;
        Position destination{ 0, 0 };
        int crateIndex = -1;
        Position crateDestination{ 0, 0 };
    };

    // Both players calculate moves from the SAME pre-turn board state.
    const auto planMove = [this](Position start, int dr, int dc) {
        MovePlan plan;
        plan.destination = Advance(start, dr, dc);
        if (IsBlocked(plan.destination)) return plan;
        plan.crateIndex = FindCrate(plan.destination);
        if (plan.crateIndex >= 0) {
            plan.crateDestination = Advance(plan.destination, dr, dc);
            if (IsBlocked(plan.crateDestination) || IsCrate(plan.crateDestination) ||
                SamePosition(plan.crateDestination, m_primary) ||
                SamePosition(plan.crateDestination, m_opposite)) return plan;
        }
        plan.allowed = true;
        return plan;
        };

    MovePlan primary = planMove(m_primary, rowDelta, colDelta);
    MovePlan opposite = planMove(m_opposite, -rowDelta, -colDelta);

    if (primary.allowed && opposite.allowed) {
        const bool sameCrate = primary.crateIndex >= 0 &&
            primary.crateIndex == opposite.crateIndex;
        const bool sameDestination = SamePosition(primary.destination, opposite.destination);
        const bool swappedPlaces = SamePosition(primary.destination, m_opposite) &&
            SamePosition(opposite.destination, m_primary);
        const bool crateConflict =
            (primary.crateIndex >= 0 && SamePosition(primary.crateDestination, opposite.destination)) ||
            (opposite.crateIndex >= 0 && SamePosition(opposite.crateDestination, primary.destination)) ||
            (primary.crateIndex >= 0 && opposite.crateIndex >= 0 &&
                SamePosition(primary.crateDestination, opposite.crateDestination));
        if (sameCrate || sameDestination || swappedPlaces || crateConflict) {
            primary.allowed = false;
            opposite.allowed = false;
        }
    }

    if (primary.allowed && !opposite.allowed && SamePosition(primary.destination, m_opposite))
        primary.allowed = false;
    if (opposite.allowed && !primary.allowed && SamePosition(opposite.destination, m_primary))
        opposite.allowed = false;
    if (!primary.allowed && !opposite.allowed) return false;

    m_history.push_back({ m_primary, m_opposite, m_crates });
    if (primary.allowed) {
        if (primary.crateIndex >= 0) m_crates[primary.crateIndex] = primary.crateDestination;
        m_primary = primary.destination;
    }
    if (opposite.allowed) {
        if (opposite.crateIndex >= 0) m_crates[opposite.crateIndex] = opposite.crateDestination;
        m_opposite = opposite.destination;
    }
    ++m_moveCount;
    return true;
}

bool ObstacleSystem::Undo() {
    if (m_history.empty()) return false;
    const Snapshot previous = m_history.back();
    m_history.pop_back();
    m_primary = previous.primary;
    m_opposite = previous.opposite;
    m_crates = previous.crates;
    --m_moveCount;
    return true;
}

void ObstacleSystem::Reset() {
    m_primary = m_primaryStart;
    m_opposite = m_oppositeStart;
    m_crates = m_initialCrates;
    m_history.clear();
    m_moveCount = 0;
}
