# Opposite Ends — Level ratings and personal bests (Stage 5)

The original pitch proposed replaying levels for stars earned by completing a
puzzle in fewer moves. Stage 5 implements this using the Stage 4 puzzle solver.

## Player-facing behavior

- On loading a level, the solver finds the shortest solution (`Par`) if it can
  complete within 20,000 states. The HUD displays **Level**, **Moves**, **Par**,
  **Best**, and the existing key labels on separate lines.
- On reaching both goals, the completion screen displays **Stars: 3/3**,
  **Stars: 2/3**, or **Stars: 1/3** based on the move count.
- **3 stars:** solve in exactly the shortest known number of moves.
- **2 stars:** solve within the shortest number + `max(2, ceil(par / 4))` moves.
- **1 star:** any slower successful completion.
- **Unknown par:** displays `?` and the completion is unrated rather than
  inventing an optimal move count. The player can still finish and save a best.
- `Z` (Undo), `R` (Reset), and `H` (Hint) continue to work exactly as before.
  Undo only changes the current effective move count. There is **no extra star
  penalty** for using Undo or asking for hints. This is a design policy that
  can be changed later after the team discusses it.
- Personal-best move counts are saved to `OppositeEnds_scores.txt` in the
  game's working directory; they are loaded at startup. Git ignores this file.
  If saving fails, the completion screen shows `Best score not saved`.

## Compatibility with evolving levels

A best is keyed by the **level number and a stable fingerprint of the level's
text**. If a teammate changes the level layout in a future commit, the old best
will not incorrectly appear as the score for the redesigned puzzle. Windows
CRLF and Unix LF line endings produce the same fingerprint.

The scoreboard uses a portable C++14 implementation (`LevelProgress.h/.cpp`).
It writes a versioned score file; a failed or malformed load never overwrites
existing in-memory records. Score saving uses a temporary file and a backup
fallback for operating systems that cannot rename over an existing file.

## Developer tools and tests

Run everything from the repository root:

```sh
sh tests/run_portable_tests.sh
```

The script checks existing movement/solver tests, the level validator, the
new scoring/save/load tests, and the authoring report for Levels 1, 2, and 5.

To inspect star thresholds for any existing text level:

```sh
clang++ -std=c++14 -O2 -Wall -Wextra -pedantic \
  tools/AnalyzeRatings.cpp OppositeEnds/ObstacleSystem.cpp \
  OppositeEnds/PuzzleSolver.cpp OppositeEnds/LevelProgress.cpp \
  -o /tmp/analyze_ratings
/tmp/analyze_ratings Levels/level01.txt Levels/level02.txt Levels/level05.txt
```

## Boundaries and integration

This patch builds on **Stage 4 Smart Hints** (it does not apply to older
`main` without those changes). The Windows renderer was not compiled in this
environment. Have the team test the HUD, completion overlay, and score-file
permissions in the Windows Visual Studio build. This feature does **not**
modify Nicholas's unfinished ice, empty-space, or game-over code.
