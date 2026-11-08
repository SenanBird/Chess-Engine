# Chess Engine

A local two-player chess app for Linux, written in C++17 with Qt 6 Widgets.
Two people can play on the same computer. Select a piece to see its legal
destinations, then select a destination to move.

## What works today

- Desktop board with SVG pieces, legal move highlights, and a promotion picker.
- Check, checkmate, stalemate, castling, en passant, and promotion.
- Automatic draws for fivefold repetition, the 75-move rule, and common
  insufficient-material positions.
- A game-over dialog and automatic saving for completed matches. Closing
  an unfinished match does not write a record.

Threefold and 50-move draw claims and general dead-position detection are
still open.

## Build and run on Linux

Install a C++ compiler with C++17 support, CMake 3.16 or newer
(or newer if required by your Qt installation), Qt 6 Widgets development
files, and Qt 6 SVG support. The app has been developed and run on CachyOS.

Open a terminal in the repository root, the folder containing this README
and `engine/`, and run:

```bash
cmake -S engine -B engine/build &&
cmake --build engine/build --parallel &&
ctest --test-dir engine/build --output-on-failure &&
cp engine/build/chess_gui ./chess_gui &&
./chess_gui
```

CMake creates `engine/build/` on your computer. Keep the built `chess_gui`
beside `engine/` so it can find the artwork. Completed games are saved
locally to `engine/data/games.tsv`.

## How the project is organized

| Path | Purpose |
|---|---|
| `engine/src/` and `engine/include/` | Shared chess rules, game state, and saving |
| `engine/gui/` | Qt desktop interface and piece artwork |
| `engine/tests/` | Regression tests for castling and draw handling |
| `engine/docs/project-notes.md` | Project status, roadmap, and developer handoff |

## What I’m building next

I’m using this project to learn C++, desktop development, and machine
learning. The next milestones are match history and replay, a random
opponent, and a Python pipeline that converts saved games and public PGN
games into training examples. The longer-term goal is to train an opponent
I can play against in the app.
