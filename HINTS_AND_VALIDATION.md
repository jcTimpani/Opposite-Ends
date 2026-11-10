# Opposite Ends — Smart Hint System and Level Validation

This feature was added on top of the text-based level loader. It preserves the
existing mirrored movement, crate-pushing, pressure plates, powered doors, and
undo/reset rules.

## In the game

Press **H** during a level to ask for one next movement. The display shows
`Hint: D (4 moves remaining)` (where `D` means Right), or reports when no
solution is reachable from the current position. It does **not** move the
characters for you, affect the move counter, or change undo history. A new
movement, Undo, Reset, or level load clears the old hint.

The solver can say **search limit reached**: this means it searched 20,000
unique states without finishing, **not** that the puzzle is impossible. Larger
crate-heavy levels may reach this limit and temporarily stall input while H is
processed, so the search should remain bounded.

The hint uses breadth-first search (BFS) with **one input as one edge**. It
copies the engine-independent `ObstacleSystem` and calls the same movement
logic as gameplay, rather than reimplementing wall, crate, or gate rules.
Crate positions are sorted in the state key because crates are identical.
The first move of a shortest found solution is suggested. The shortest path
count includes only successful keypresses, not Undo or Reset.

## Test and validate on macOS or Linux

From the project root, with clang++ installed:

```sh
sh tests/run_portable_tests.sh
```

This compiles and runs engine-independent regression tests, then compiles
`tools/ValidateLevels.cpp` and checks `level01.txt`, `level02.txt`, and
`level05.txt`. It doesn't compile the Windows renderer.

Individual validation of any text level:

```sh
clang++ -std=c++14 -O2 -Wall -Wextra -pedantic \
  tools/ValidateLevels.cpp OppositeEnds/ObstacleSystem.cpp \
  OppositeEnds/PuzzleSolver.cpp -o /tmp/validate_levels
/tmp/validate_levels Levels/level01.txt Levels/level02.txt Levels/level05.txt
```

`SOLVABLE` prints the shortest input sequence; `UNSOLVABLE` means BFS exhausted
the reachable states; `UNKNOWN` means the 50,000-state validation cap was hit.
Nonzero exit status indicates at least one invalid/unsolved/unknown level.
Use this during level design, but **do not call UNKNOWN unsolvable**.

## New example level (level05.txt)

`Levels/level05.txt` is a small reproducible crate + plate + powered gate
example. From its initial state press `D` four times to solve it. On the first
press red pushes a crate onto a plate; the gate becomes open; subsequent presses
push the crate through the gate while both astronauts move to their goals.

Levels 1 and 2 are unchanged. Level 5 is validated by the command-line
checker but will not appear in sequential gameplay until levels 3 and 4 exist
in the main branch. After those levels are merged, ensure the game runtime
working directory can find `Levels/level05.txt`. No new game-engine dependencies or textures are needed.

## Integration and limitations

`PuzzleSolver.cpp`/`.h` have been included in `.vcxproj` and `.filters`.
The algorithm and regression tests compile with C++14; a full Windows
Visual Studio / DirectX / LARC build has **not** been run in this environment.
Have a Windows teammate verify pressing H, readable HUD text, and level 5.

Design note: H is a player-assistance feature. Whether it should count against
future star ratings is a separate team decision; this implementation does not
introduce or modify a star-scoring system.
