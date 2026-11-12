#pragma once

#include "ObstacleSystem.h"

#include <cstddef>
#include <string>

// Finds shortest input sequences using the exact same rules as gameplay.
// Search never changes the live game state, its undo history, or its move counter.
class PuzzleSolver {
public:
    enum class Status { Solved, NoSolution, StateLimit };

    struct Result {
        Status status = Status::NoSolution;
        std::string moves; // W/A/S/D inputs, including the first recommended move.
        std::size_t statesVisited = 0;
    };

    // Bounds the search so a large crate puzzle cannot grow without limit.
    static Result Solve(const ObstacleSystem& initial, std::size_t maxStates = 20000);

private:
    static std::string StateKey(const ObstacleSystem& state);
};
