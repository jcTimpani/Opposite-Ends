#include "ObstacleSystem.h"

#include <cstdlib> // std::abs

namespace {
ObstacleSystem::Position Advance(const ObstacleSystem::Position& p, int dr, int dc) {
    return { p.row + dr, p.col + dc };
}
}

ObstacleSystem::ObstacleSystem() = default;

bool ObstacleSystem::SamePosition(const Position& a, const Position& b) {
    return a.row == b.row && a.col == b.col;
}

bool ObstacleSystem::IsWall(const Position& position) const {
    // Orange outer border and purple walls are all solid.
    if (position.row <= 0 || position.row >= kGridSize - 1 ||
        position.col <= 0 || position.col >= kGridSize - 1) {
        return true;
    }

    static const Position walls[] = {
        { 6, 4 }, { 6, 5 }, { 6, 6 }, { 6, 7 }, { 6, 8 },
        { 11, 10 }, { 12, 10 }, { 13, 10 }
    };
    for (const Position& wall : walls) {
        if (SamePosition(position, wall)) {
            return true;
        }
    }
    return false;
}

// Two plates control one door. One plate is accessible to the primary
// astronaut immediately, while the other can be held down by a pushed crate.
bool ObstacleSystem::IsPlate(const Position& position) const {
    static const Position plates[] = { { 5, 4 }, { 4, 7 } };
    for (const Position& plate : plates) {
        if (SamePosition(position, plate)) {
            return true;
        }
    }
    return false;
}

bool ObstacleSystem::IsPlateOccupied(const Position& position) const {
    return IsPlate(position) &&
        (SamePosition(position, m_primary) ||
         SamePosition(position, m_opposite) ||
         IsCrate(position));
}

bool ObstacleSystem::IsDoor(const Position& position) const {
    return SamePosition(position, Position{ 12, 13 });
}

bool ObstacleSystem::IsPlatePressed() const {
    static const Position plates[] = { { 5, 4 }, { 4, 7 } };
    for (const Position& plate : plates) {
        if (IsPlateOccupied(plate)) {
            return true;
        }
    }
    return false;
}

bool ObstacleSystem::IsDoorOccupied() const {
    return IsDoor(m_primary) || IsDoor(m_opposite) || IsCrate({ 12, 13 });
}

bool ObstacleSystem::IsDoorOpen() const {
    // Keep the gate open while something is inside so it never closes onto
    // an astronaut or crate after a pressure plate is released.
    return IsPlatePressed() || IsDoorOccupied();
}

bool ObstacleSystem::IsBlocked(const Position& position) const {
    return IsWall(position) || (IsDoor(position) && !IsDoorOpen());
}

int ObstacleSystem::FindCrate(const Position& position) const {
    for (int i = 0; i < kCrateCount; ++i) {
        if (SamePosition(position, m_crates[i])) {
            return i;
        }
    }
    return -1;
}

bool ObstacleSystem::IsCrate(const Position& position) const {
    return FindCrate(position) != -1;
}

bool ObstacleSystem::TryMove(int rowDelta, int colDelta) {
    // Reject diagonals and zero steps, even if a caller supplies them.
    if (std::abs(rowDelta) + std::abs(colDelta) != 1) {
        return false;
    }

    struct MovePlan {
        bool allowed = false;
        Position destination{ 0, 0 };
        int crateIndex = -1;
        Position crateDestination{ 0, 0 };
    };

    // Plan both turns from the SAME original state. Do not move either
    // character (or a crate) until conflicts have been resolved.
    const auto planMove = [this](Position start, int dr, int dc) {
        MovePlan plan;
        plan.destination = Advance(start, dr, dc);
        // Use the switch state at the START of the input for both players.
        // A plate newly pressed on this turn opens the gate on the next turn.
        if (IsBlocked(plan.destination)) {
            return plan;
        }

        plan.crateIndex = FindCrate(plan.destination);
        if (plan.crateIndex >= 0) {
            plan.crateDestination = Advance(plan.destination, dr, dc);
            // Single-crate pushes only. A crate cannot enter a wall, another
            // crate, or either character's occupied starting tile.
            if (IsBlocked(plan.crateDestination) ||
                IsCrate(plan.crateDestination) ||
                SamePosition(plan.crateDestination, m_primary) ||
                SamePosition(plan.crateDestination, m_opposite)) {
                return plan;
            }
        }
        plan.allowed = true;
        return plan;
    };

    MovePlan primary = planMove(m_primary, rowDelta, colDelta);
    MovePlan opposite = planMove(m_opposite, -rowDelta, -colDelta);

    if (primary.allowed && opposite.allowed) {
        // Both characters must not occupy the same square, push the same
        // crate from opposite directions, or produce colliding crate moves.
        const bool sameCrate = primary.crateIndex >= 0 &&
            primary.crateIndex == opposite.crateIndex;
        const bool sameDestination = SamePosition(primary.destination, opposite.destination);
        const bool swappedPlaces = SamePosition(primary.destination, m_opposite) &&
            SamePosition(opposite.destination, m_primary);
        const bool crateConflict =
            (primary.crateIndex >= 0 &&
                SamePosition(primary.crateDestination, opposite.destination)) ||
            (opposite.crateIndex >= 0 &&
                SamePosition(opposite.crateDestination, primary.destination)) ||
            (primary.crateIndex >= 0 && opposite.crateIndex >= 0 &&
                SamePosition(primary.crateDestination, opposite.crateDestination));

        if (sameCrate || sameDestination || swappedPlaces || crateConflict) {
            primary.allowed = false;
            opposite.allowed = false;
        }
    }

    // A character also cannot walk into a character that is stuck.
    if (primary.allowed && !opposite.allowed &&
        SamePosition(primary.destination, m_opposite)) {
        primary.allowed = false;
    }
    if (opposite.allowed && !primary.allowed &&
        SamePosition(opposite.destination, m_primary)) {
        opposite.allowed = false;
    }

    if (!primary.allowed && !opposite.allowed) {
        return false;
    }

    m_history.push_back({ m_primary, m_opposite, m_crates });
    if (primary.allowed) {
        if (primary.crateIndex >= 0) {
            m_crates[primary.crateIndex] = primary.crateDestination;
        }
        m_primary = primary.destination;
    }
    if (opposite.allowed) {
        if (opposite.crateIndex >= 0) {
            m_crates[opposite.crateIndex] = opposite.crateDestination;
        }
        m_opposite = opposite.destination;
    }
    ++m_moveCount;
    return true;
}

bool ObstacleSystem::Undo() {
    if (m_history.empty()) {
        return false;
    }
    const Snapshot previous = m_history.back();
    m_history.pop_back();
    m_primary = previous.primary;
    m_opposite = previous.opposite;
    m_crates = previous.crates;
    --m_moveCount;
    return true;
}

void ObstacleSystem::Reset() {
    m_primary = { 4, 4 };
    m_opposite = { 13, 13 };
    m_crates = {{ { 4, 6 }, { 13, 12 }, { 9, 9 } }};
    m_history.clear();
    m_moveCount = 0;
}
