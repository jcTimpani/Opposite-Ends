// Portable authoring tool: show each level's par and three-star thresholds.
#include "../OppositeEnds/LevelProgress.h"
#include "../OppositeEnds/ObstacleSystem.h"
#include "../OppositeEnds/PuzzleSolver.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: analyze_ratings Levels/level01.txt [more levels...]\n";
        return 2;
    }
    bool allSolved = true;
    for (int i = 1; i < argc; ++i) {
        ObstacleSystem board;
        std::string error;
        if (!board.LoadFromFile(argv[i], &error)) {
            std::cerr << argv[i] << ": INVALID: " << error << '\n';
            allSolved = false;
            continue;
        }
        const auto result = PuzzleSolver::Solve(board, 50000);
        if (result.status != PuzzleSolver::Status::Solved) {
            std::cout << argv[i] << ": " <<
                (result.status == PuzzleSolver::Status::StateLimit ? "UNKNOWN (search cap)" : "UNSOLVABLE") << '\n';
            allSolved = false;
            continue;
        }
        const int par = static_cast<int>(result.moves.size());
        int maxTwoStar = par;
        while (LevelProgress::StarsFor(maxTwoStar + 1, par) == 2) ++maxTwoStar;
        std::cout << argv[i] << ": par=" << par
                  << "  3 stars=" << par << " moves"
                  << "  2 stars=" << (par + 1) << "-" << maxTwoStar
                  << "  1 star=" << (maxTwoStar + 1) << "+ moves\n";
    }
    return allSolved ? 0 : 1;
}
