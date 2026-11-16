#include "../OppositeEnds/LevelProgress.h"
#include "../OppositeEnds/ObstacleSystem.h"
#include "../OppositeEnds/PuzzleSolver.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace {
void Write(const std::string& name, const std::string& contents) {
    std::ofstream out(name, std::ios::binary);
    assert(out.good());
    out << contents;
    assert(out.good());
}
}

int main() {
    assert(LevelProgress::StarsFor(4, 4) == 3);
    assert(LevelProgress::StarsFor(5, 4) == 2);
    assert(LevelProgress::StarsFor(6, 4) == 2);
    assert(LevelProgress::StarsFor(7, 4) == 1);
    assert(LevelProgress::StarsFor(14, 11) == 2);
    assert(LevelProgress::StarsFor(15, 11) == 1);
    assert(LevelProgress::StarsFor(3, 4) == 0);
    assert(LevelProgress::StarsFor(4, -1) == 0);
    assert(LevelProgress::StarsFor(-1, 4) == 0);

    const std::string file = "opposite_ends_score_test.txt";
    const std::string level = "opposite_ends_fingerprint_test.txt";
    Write(level, "reb\nceh\n");
    const auto original = LevelProgress::FingerprintFile(level);
    assert(original != 0);
    Write(level, "reb\r\nceh\r\n");
    assert(LevelProgress::FingerprintFile(level) == original);
    Write(level, "rwb\nceh\n");
    const auto revised = LevelProgress::FingerprintFile(level);
    assert(revised != original);

    LevelProgress scores;
    assert(!scores.Load("a_file_that_does_not_exist_oe_test.txt"));
    assert(scores.BestMoves(1, original) == -1);
    assert(scores.RecordCompletion(1, original, 9));
    assert(!scores.RecordCompletion(1, original, 11));
    assert(scores.RecordCompletion(1, original, 6));
    assert(scores.RecordCompletion(1, revised, 12));
    assert(scores.RecordCompletion(2, original, 20));
    assert(scores.BestMoves(1, original) == 6);
    assert(scores.BestMoves(1, revised) == 12);
    assert(scores.BestMoves(2, original) == 20);
    assert(scores.BestMoves(3, original) == -1);
    assert(!scores.RecordCompletion(1, 0, 5));
    assert(!scores.RecordCompletion(-1, original, 5));
    assert(!scores.RecordCompletion(1, original, -5));
    assert(scores.Save(file));

    LevelProgress restored;
    assert(restored.Load(file));
    assert(restored.BestMoves(1, original) == 6);
    assert(restored.BestMoves(1, revised) == 12);
    assert(restored.BestMoves(2, original) == 20);
    assert(restored.RecordCompletion(1, revised, 10));
    assert(restored.Save(file));
    LevelProgress reopened;
    assert(reopened.Load(file));
    assert(reopened.BestMoves(1, revised) == 10);

    Write(file, "OPPOSITE_ENDS_SCORES_V1\r\n1 99 3\r\n");
    assert(reopened.Load(file));
    assert(reopened.BestMoves(1, 99) == 3);
    Write(file, "INVALID_HEADER\n1 99 3\n");
    assert(!reopened.Load(file));
    assert(reopened.BestMoves(1, 99) == 3);
    Write(file, "OPPOSITE_ENDS_SCORES_V1\n1 99 garbage\n");
    assert(!reopened.Load(file));
    assert(reopened.BestMoves(1, 99) == 3);

    // Real game integration: solve Level 1, record its exact move count,
    // then Undo and replay a move without changing the final rating.
    ObstacleSystem board;
    assert(board.LoadFromFile("Levels/level01.txt"));
    const auto solved = PuzzleSolver::Solve(board);
    assert(solved.status == PuzzleSolver::Status::Solved);
    assert(solved.moves == "SSDD");
    for (char key : solved.moves) {
        if (key == 'S') assert(board.TryMove(1, 0));
        if (key == 'D') assert(board.TryMove(0, 1));
    }
    assert(board.IsWon());
    assert(board.MoveCount() == 4);
    assert(LevelProgress::StarsFor(board.MoveCount(), static_cast<int>(solved.moves.size())) == 3);
    assert(board.Undo());
    assert(board.MoveCount() == 3);
    assert(!board.IsWon());
    assert(board.TryMove(0, 1));
    assert(board.IsWon());
    assert(board.MoveCount() == 4);

    std::remove(level.c_str());
    std::remove(file.c_str());
    std::cout << "Level progress, 3-star scoring, and save/load tests passed.\n";
}
