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

## Stage 3 - pressure plates and powered gate

This prototype adds two floor pressure plates at `(row 5, col 4)` and
`(row 4, col 7)` and one powered gate at `(row 12, col 13)`.

- The gate is **closed by default** (red tile). It opens (green tile) if either
  plate is occupied by either astronaut **or** by a crate.
- Both mirrored movement plans use the gate's state **at the start of the
  input**; stepping onto a plate on a turn opens the gate for the following
  input, not the same input.
- A gate remains open when an astronaut or crate occupies its cell, preventing
  it from closing onto an object when the last plate is released.
- Undo and reset restore players/crates, and the gate recomputes its state from
  the restored positions, so no extra gate-state history is required.
- Pressure plates and the gate are represented using tinted versions of the
  existing `tile` texture. No sprite or Windows project changes are needed.
- The HUD now uses two shorter labels within the screen bounds.

### Quick Windows demonstration

1. From the starting screen press **Enter**, then press **S** once: the primary
   astronaut stands on the yellow pressure plate, turning it green. The gate
   turns green, but the opposite astronaut remains at its start (the gate
   was closed at the beginning of that input).
2. Press **S** again: the primary astronaut remains blocked by the wall while
   the opposite astronaut enters the now-open gate.
3. Press **Z** twice: the gate re-closes after the primary astronaut leaves the
   plate; the move counter returns to zero.
4. Press **R** to reset, then **D** twice: a pushed crate activates the second
   plate and opens the gate. Press **D** twice more: on the third input the
   astronaut takes the crate's place on the plate, and on the fourth it steps
   off the plate so the door closes. Press **Z** to restore the open gate.

Re-run `tests/ObstacleSystemTests.cpp` with a C++14 compiler to validate both
new switch behaviors along with the original obstacle behavior. The Windows
engine render/HUD must still be checked inside Visual Studio.
