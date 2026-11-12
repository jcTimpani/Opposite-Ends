// Portable level-checking tool; compile with a C++14 compiler.
// Usage: ./validate_levels Levels/level01.txt Levels/level02.txt
#include "../OppositeEnds/ObstacleSystem.h"
#include "../OppositeEnds/PuzzleSolver.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: validate_levels <level-file> [level-file ...]\n";
        return 2;
    }

    bool allSolved = true;
    for (int i = 1; i < argc; ++i) {
        ObstacleSystem board;
        std::string error;
        if (!board.LoadFromFile(argv[i], &error)) {
            std::cerr << argv[i] << ": invalid level: " << error << '\n';
            allSolved = false;
            continue;
        }
        const auto result = PuzzleSolver::Solve(board, 50000);
        if (result.status == PuzzleSolver::Status::Solved) {
            std::cout << argv[i] << ": SOLVABLE, shortest input sequence "
                      << result.moves.size() << " moves, path=" << result.moves
                      << ", states=" << result.statesVisited << '\n';
        }
        else {
            std::cerr << argv[i] << ": "
                      << (result.status == PuzzleSolver::Status::StateLimit
                              ? "UNKNOWN: search bound exceeded" : "UNSOLVABLE")
                      << ", states=" << result.statesVisited << '\n';
            allSolved = false;
        }
    }
    return allSolved ? 0 : 1;
}
