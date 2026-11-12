// Portable regression tests for the text-file based puzzle system.
// clang++ -std=c++14 -Wall -Wextra -pedantic tests/ObstacleSystemTests.cpp \
//   OppositeEnds/ObstacleSystem.cpp OppositeEnds/PuzzleSolver.cpp -o /tmp/ops_tests && /tmp/ops_tests
#include "../OppositeEnds/ObstacleSystem.h"
#include "../OppositeEnds/PuzzleSolver.h"

#include <cassert>
#include <iostream>
#include <string>

using Position = ObstacleSystem::Position;

static bool At(Position p, int row, int col) {
    return p.row == row && p.col == col;
}

static void ApplyPath(ObstacleSystem& board, const std::string& path) {
    for (const char c : path) {
        int dr = 0, dc = 0;
        if (c == 'W') dr = -1;
        else if (c == 'A') dc = -1;
        else if (c == 'S') dr = 1;
        else if (c == 'D') dc = 1;
        else assert(false && "Unknown move from solver");
        assert(board.TryMove(dr, dc));
    }
}

static void TestLevelParsing() {
    ObstacleSystem board;
    std::string error;
    assert(board.LoadFromText("eeeeee\nerecee\neehebe\neeeeee\n", &error));
    assert(board.Width() == 6 && board.Height() == 4);
    assert(At(board.PrimaryPosition(), 1, 1));
    assert(At(board.OppositePosition(), 2, 4));
    assert(At(board.PrimaryGoal(), 1, 3));
    assert(At(board.OppositeGoal(), 2, 2));

    // Invalid maps cannot corrupt or replace the live puzzle.
    assert(!board.LoadFromText("eee\nerb\n", &error));
    assert(!error.empty());
    assert(board.Width() == 6 && board.Height() == 4);
    assert(At(board.PrimaryPosition(), 1, 1));
    assert(!board.LoadFromText("erch\ne!be\n", &error));
    assert(!board.LoadFromText("rcbh\nww\n", &error));
    assert(board.LoadFromText("eeeeee\r\nerecee\r\neehebe\r\neeeeee\r\n", &error));
    assert(error.empty());
}

static void TestWallsCratesPlatesAndUndo() {
    // Every object in this test is defined by the level text, not hard-coded coordinates.
    ObstacleSystem board;
    const std::string map =
        "eeeeeeee\n"
        "erkpdcee\n"
        "eeeeeeee\n"
        "heeebeee\n"
        "eeeeeeee\n";
    assert(board.LoadFromText(map));
    assert(board.CrateCount() == 1);
    assert(board.IsCrate({ 1, 2 }));
    assert(board.IsPlate({ 1, 3 }));
    assert(board.IsDoor({ 1, 4 }));
    assert(!board.IsDoorOpen());
    assert(!board.TryMove(0, 0));
    assert(!board.TryMove(1, 1));

    assert(board.TryMove(0, 1)); // Red pushes crate onto plate.
    assert(At(board.PrimaryPosition(), 1, 2));
    assert(At(board.OppositePosition(), 3, 3));
    assert(board.IsCrate({ 1, 3 }));
    assert(board.IsPlatePressed());
    assert(board.IsDoorOpen());
    assert(board.MoveCount() == 1);

    assert(board.TryMove(0, 1)); // Crate enters the now-open gate.
    assert(board.IsCrate({ 1, 4 }));
    assert(board.IsPlateOccupied({ 1, 3 })); // Astronaut holds the switch.
    assert(board.IsDoorOpen());

    assert(board.TryMove(0, 1)); // Astronaut enters the door tile.
    assert(board.IsCrate({ 1, 5 }));
    assert(At(board.PrimaryPosition(), 1, 4));
    assert(board.IsDoorOpen()); // Occupied gate does not close on the player.

    assert(board.Undo());
    assert(At(board.PrimaryPosition(), 1, 3));
    assert(board.IsCrate({ 1, 4 }));
    assert(board.MoveCount() == 2);
    board.Reset();
    assert(At(board.PrimaryPosition(), 1, 1));
    assert(board.IsCrate({ 1, 2 }));
    assert(!board.IsDoorOpen());
    assert(board.MoveCount() == 0);
    assert(!board.Undo());
    assert(board.IsWall({ -1, 0 }));
    assert(board.IsWall({ 5, 0 }));
}

static void TestSolver() {
    ObstacleSystem board;
    assert(board.LoadFromText("eeeeee\nerecee\neehebe\neeeeee\n"));
    const auto shortPath = PuzzleSolver::Solve(board);
    assert(shortPath.status == PuzzleSolver::Status::Solved);
    assert(shortPath.moves == "DD");
    assert(board.MoveCount() == 0); // A hint must not play the game for the user.
    assert(At(board.PrimaryPosition(), 1, 1));
    assert(!board.Undo());
    ApplyPath(board, shortPath.moves);
    assert(board.IsWon());

    // Search from a partial game must preserve the player's history.
    board.Reset();
    assert(board.TryMove(0, 1));
    const auto partial = PuzzleSolver::Solve(board);
    assert(partial.status == PuzzleSolver::Status::Solved);
    assert(partial.moves == "D");
    assert(board.MoveCount() == 1);
    assert(board.Undo());
    assert(At(board.PrimaryPosition(), 1, 1));

    const auto limited = PuzzleSolver::Solve(board, 1);
    assert(limited.status == PuzzleSolver::Status::StateLimit);
    assert(limited.statesVisited == 1);

    // Sealed goal: exhaust the state space without guessing that a solution exists.
    assert(board.LoadFromText("wwwww\nwrcww\nwbwhw\nwwwww\n"));
    const auto impossible = PuzzleSolver::Solve(board);
    assert(impossible.status == PuzzleSolver::Status::NoSolution);
}

static void TestRepositoryLevels() {
    const char* files[] = {
        "Levels/level01.txt", "Levels/level02.txt", "Levels/level05.txt"
    };
    for (const char* name : files) {
        ObstacleSystem board;
        std::string error;
        assert(board.LoadFromFile(name, &error));
        const auto result = PuzzleSolver::Solve(board, 50000);
        assert(result.status == PuzzleSolver::Status::Solved);
        const std::size_t length = result.moves.size();
        assert(length > 0);
        ApplyPath(board, result.moves);
        assert(board.IsWon());
        assert(static_cast<std::size_t>(board.MoveCount()) == length);
        // Reset restores a replayable starting position.
        board.Reset();
        assert(board.MoveCount() == 0);
        assert(!board.IsWon());
        ApplyPath(board, result.moves);
        assert(board.IsWon());
        std::cout << name << ": " << length << " moves, path=" << result.moves << '\n';
    }
}

int main() {
    TestLevelParsing();
    TestWallsCratesPlatesAndUndo();
    TestSolver();
    TestRepositoryLevels();
    std::cout << "All loader, obstacle, gate, undo, hint and solvability tests passed.\n";
}
