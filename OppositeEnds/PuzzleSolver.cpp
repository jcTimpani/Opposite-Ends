#include "PuzzleSolver.h"

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
struct Direction { int dr, dc; char key; };
const Direction directions[] = {
    { -1, 0, 'W' }, { 0, -1, 'A' }, { 1, 0, 'S' }, { 0, 1, 'D' }
};
}

std::string PuzzleSolver::StateKey(const ObstacleSystem& state) {
    // Crates are indistinguishable. Sort their cell IDs to avoid exploring
    // different permutations of the exact same board configuration.
    std::vector<int> crates;
    crates.reserve(state.m_crates.size());
    for (const auto& crate : state.m_crates)
        crates.push_back(crate.row * ObstacleSystem::kGridSize + crate.col);
    std::sort(crates.begin(), crates.end());

    std::string key;
    key.reserve(4 + crates.size() * 2);
    const auto appendCell = [&key](int id) {
        key.push_back(static_cast<char>(id & 0xff));
        key.push_back(static_cast<char>((id >> 8) & 0xff));
    };
    appendCell(state.m_primary.row * ObstacleSystem::kGridSize + state.m_primary.col);
    appendCell(state.m_opposite.row * ObstacleSystem::kGridSize + state.m_opposite.col);
    for (const int cell : crates) appendCell(cell);
    return key;
}

PuzzleSolver::Result PuzzleSolver::Solve(const ObstacleSystem& initial, std::size_t maxStates) {
    Result result;
    if (initial.Width() == 0 || maxStates == 0) {
        result.status = Status::StateLimit;
        return result;
    }

    // No copy of the player's undo stack is retained in the search graph.
    ObstacleSystem startingState = initial;
    startingState.m_history.clear();
    if (startingState.IsWon()) {
        result.status = Status::Solved;
        result.statesVisited = 1;
        return result;
    }

    struct Node {
        ObstacleSystem board;
        std::size_t parent;
        char action;
    };
    std::vector<Node> nodes;
    nodes.reserve(std::min<std::size_t>(maxStates, 20000));
    nodes.push_back({ std::move(startingState), 0, '\0' });
    std::unordered_set<std::string> seen;
    seen.insert(StateKey(nodes.front().board));

    // Breadth-first search guarantees a minimum-input solution, not simply
    // a path that looks good locally for either one astronaut.
    for (std::size_t head = 0; head < nodes.size(); ++head) {
        for (const Direction& d : directions) {
            ObstacleSystem next = nodes[head].board;
            if (!next.TryMoveInternal(d.dr, d.dc, false)) continue;
            const std::string key = StateKey(next);
            if (seen.find(key) != seen.end()) continue;
            if (nodes.size() >= maxStates) {
                result.status = Status::StateLimit;
                result.statesVisited = nodes.size();
                return result;
            }
            const bool won = next.IsWon();
            const std::size_t nextIndex = nodes.size();
            seen.insert(key);
            nodes.push_back({ std::move(next), head, d.key });

            if (won) {
                std::string reversed;
                for (std::size_t i = nextIndex; i != 0; i = nodes[i].parent)
                    reversed.push_back(nodes[i].action);
                result.moves.assign(reversed.rbegin(), reversed.rend());
                result.status = Status::Solved;
                result.statesVisited = nodes.size();
                return result;
            }
        }
    }
    result.status = Status::NoSolution;
    result.statesVisited = nodes.size();
    return result;
}
