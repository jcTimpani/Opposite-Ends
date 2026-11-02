// Compile without the Windows/DirectX game engine:
// clang++ -std=c++17 -Wall -Wextra -pedantic tests/ObstacleSystemTests.cpp OppositeEnds/ObstacleSystem.cpp -o /tmp/obstacle_tests && /tmp/obstacle_tests
#include "../OppositeEnds/ObstacleSystem.h"

#include <cassert>
#include <iostream>

using Position = ObstacleSystem::Position;

bool IsAt(Position actual, int row, int col) {
    return actual.row == row && actual.col == col;
}

int main() {
    ObstacleSystem game;
    assert(IsAt(game.PrimaryPosition(), 4, 4));
    assert(IsAt(game.OppositePosition(), 13, 13));
    assert(game.IsWall({ 6, 4 }));
    assert(game.IsWall({ 13, 10 }));
    assert(game.IsWall({ 0, 4 }));
    assert(!game.IsWall({ 4, 4 }));
    assert(game.IsCrate({ 4, 6 }));
    assert(game.IsCrate({ 13, 12 }));
    assert(!game.TryMove(0, 0));
    assert(!game.TryMove(1, 1));

    // First right input moves primary and pushes the opposite astronaut's crate.
    assert(game.TryMove(0, 1));
    assert(IsAt(game.PrimaryPosition(), 4, 5));
    assert(IsAt(game.OppositePosition(), 13, 12));
    assert(game.IsCrate({ 13, 11 }));
    assert(game.MoveCount() == 1);

    // Next input pushes the primary crate, but the opposite crate hits a wall.
    assert(game.TryMove(0, 1));
    assert(IsAt(game.PrimaryPosition(), 4, 6));
    assert(IsAt(game.OppositePosition(), 13, 12));
    assert(game.IsCrate({ 4, 7 }));
    assert(game.IsCrate({ 13, 11 }));
    assert(game.MoveCount() == 2);

    // Undo restores BOTH astronaut and crate positions, not just the player.
    assert(game.Undo());
    assert(IsAt(game.PrimaryPosition(), 4, 5));
    assert(IsAt(game.OppositePosition(), 13, 12));
    assert(game.IsCrate({ 4, 6 }));
    assert(game.MoveCount() == 1);

    game.Reset();
    assert(IsAt(game.PrimaryPosition(), 4, 4));
    assert(IsAt(game.OppositePosition(), 13, 13));
    assert(game.IsCrate({ 4, 6 }));
    assert(game.IsCrate({ 13, 12 }));
    assert(game.MoveCount() == 0);
    assert(!game.Undo());

    // Wall holds primary still; plate opens the gate for opposite on turn 2.
    assert(game.TryMove(1, 0));
    assert(game.TryMove(1, 0));
    assert(IsAt(game.PrimaryPosition(), 5, 4));
    assert(IsAt(game.OppositePosition(), 12, 13));

    // Undo the partial movement and reset again.
    assert(game.Undo());
    assert(IsAt(game.PrimaryPosition(), 5, 4));
    assert(IsAt(game.OppositePosition(), 13, 13));
    game.Reset();

    // Repeated pushes eventually stop at the solid outer boundary.
    for (int i = 0; i < 25; ++i) {
        game.TryMove(0, 1);
    }
    assert(IsAt(game.PrimaryPosition(), 4, 15));
    assert(game.IsCrate({ 4, 16 }));
    assert(!game.TryMove(0, 1));

    // Stage 3: the door is initially closed, so the first downward input
    // blocks the opposite astronaut while primary steps on the first plate.
    game.Reset();
    assert(game.IsDoor({ 12, 13 }));
    assert(game.IsPlate({ 5, 4 }));
    assert(game.IsPlate({ 4, 7 }));
    assert(!game.IsDoorOpen());
    assert(!game.IsPlatePressed());
    assert(game.TryMove(1, 0));
    assert(IsAt(game.PrimaryPosition(), 5, 4));
    assert(IsAt(game.OppositePosition(), 13, 13));
    assert(game.IsPlatePressed());
    assert(game.IsPlateOccupied({ 5, 4 }));
    assert(!game.IsPlateOccupied({ 4, 7 }));
    assert(game.IsDoorOpen());
    assert(game.MoveCount() == 1);

    // On the NEXT input the already-open gate allows the opposite player
    // through while the primary player is held still by the purple wall.
    assert(game.TryMove(1, 0));
    assert(IsAt(game.PrimaryPosition(), 5, 4));
    assert(IsAt(game.OppositePosition(), 12, 13));
    assert(game.IsDoorOpen());
    assert(game.MoveCount() == 2);

    // Undo restores gate state implicitly from restored board positions.
    assert(game.Undo());
    assert(IsAt(game.OppositePosition(), 13, 13));
    assert(game.IsDoorOpen());
    assert(game.Undo());
    assert(IsAt(game.PrimaryPosition(), 4, 4));
    assert(!game.IsPlatePressed());
    assert(!game.IsDoorOpen());
    assert(game.MoveCount() == 0);

    // A crate can latch a plate even when neither astronaut stands on it.
    game.Reset();
    assert(game.TryMove(0, 1));
    assert(game.TryMove(0, 1));
    assert(game.IsCrate({ 4, 7 }));
    assert(game.IsDoorOpen());
    assert(game.IsPlatePressed());
    assert(game.IsPlateOccupied({ 4, 7 }));
    // Pushing the crate off the plate does not close the gate if the
    // astronaut moves onto that same plate in the same turn.
    assert(game.TryMove(0, 1));
    assert(game.IsCrate({ 4, 8 }));
    assert(game.IsPlateOccupied({ 4, 7 }));
    assert(game.IsDoorOpen());
    // The following push finally clears the plate, closing the gate.
    assert(game.TryMove(0, 1));
    assert(game.IsCrate({ 4, 9 }));
    assert(!game.IsPlateOccupied({ 4, 7 }));
    assert(!game.IsDoorOpen());
    assert(!game.IsPlatePressed());
    assert(game.Undo());
    assert(game.IsCrate({ 4, 8 }));
    assert(game.IsDoorOpen());
    assert(game.Undo());
    assert(game.IsCrate({ 4, 7 }));
    assert(game.IsDoorOpen());
    game.Reset();
    assert(!game.IsDoorOpen());

    // Repeated turns must never leave a player inside a wall or crate.
    game.Reset();
    for (int turn = 0; turn < 5000; ++turn) {
        const int direction = (turn * 17 + turn / 13) % 4;
        const int dr = direction == 0 ? 1 : direction == 1 ? -1 : 0;
        const int dc = direction == 2 ? 1 : direction == 3 ? -1 : 0;
        game.TryMove(dr, dc);
        assert(!game.IsWall(game.PrimaryPosition()));
        assert(!game.IsWall(game.OppositePosition()));
        assert(!game.IsDoor(game.PrimaryPosition()) || game.IsDoorOpen());
        assert(!game.IsDoor(game.OppositePosition()) || game.IsDoorOpen());
        assert(!game.IsCrate(game.PrimaryPosition()));
        assert(!game.IsCrate(game.OppositePosition()));
        assert(!IsAt(game.PrimaryPosition(), game.OppositePosition().row,
                     game.OppositePosition().col));
        int crates = 0;
        for (int row = 0; row < ObstacleSystem::kGridSize; ++row) {
            for (int col = 0; col < ObstacleSystem::kGridSize; ++col) {
                if (game.IsCrate({ row, col })) {
                    ++crates;
                    assert(!game.IsWall({ row, col }));
                }
            }
        }
        assert(crates == 3);
    }

    std::cout << "All wall, crate, pressure plate, gate, undo, and reset tests passed.\n";
}
