# Opposite Ends — Obstacle System (Stage 2)

This feature adds a portable C++ `ObstacleSystem` that manages the two astronauts' tile positions, fixed walls, movable crates, undo, reset, and the move counter. `Game.cpp` continues to handle keyboard input, sprite-facing directions, and rendering. This change deliberately does **not** introduce goal tiles or modify astronaut art.

## Controls

- W/A/S/D or arrow keys: move primary normally and opposite character in the inverse direction.
- Z: undo the last successful movement, including crate positions and astronaut facing direction.
- R: reset both characters and crates to their starting positions.
- The HUD shows the successful move count.

## Appearance / prototype layout

- Orange border tiles: impassable boundary.
- Purple tiles: fixed wall obstacles. Positions are in `ObstacleSystem::IsWall`.
- Small gold squares: movable crate placeholders (no additional textures required). Initial positions are in `ObstacleSystem.h` and `Reset()`.

Crates can be pushed one square when the square beyond is free. Crates cannot be pulled, pushed into other crates/walls, or pushed onto a character's starting position. Both astronauts' movement plans are calculated from the same board state; conflicting simultaneous moves are rejected. If only one astronaut is blocked, the other can still move.

## Portable tests (macOS / Linux with clang++)

From the repository root:

```bash
clang++ -std=c++14 -Wall -Wextra -pedantic \
  tests/ObstacleSystemTests.cpp OppositeEnds/ObstacleSystem.cpp \
  -o /tmp/opposite_ends_obstacle_tests
/tmp/opposite_ends_obstacle_tests
```

Tests exercise walls, mirrored crate pushes, partial blocking, multi-step pushing, reset, undo, and borders. No game-engine dependencies are needed for these logic tests.

## Full game build

The Windows Visual Studio project file now includes `ObstacleSystem.cpp` and `ObstacleSystem.h`. To run the actual game, build the solution on Windows with the same LARC engine and DirectX dependencies previously used by your team. Mac-native compilation of the full game is not supported by the existing project configuration.

## Team integration

This code owns only the obstacle/puzzle movement state. The teammate adding goal tiles can query `PrimaryPosition()` and `OppositePosition()`, and check wall/crate occupancy through `IsWall()` and `IsCrate()`. The hardcoded test layout can later be replaced with per-level map data.
