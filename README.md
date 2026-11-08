# ZergChess

ZergChess is an offline desktop chess app written in C++17 with a Qt 6 Widgets interface. Play locally against another person, a random opponent, or a CPU chess AI. You can also review moves during a game and replay saved games.

The project is under active development. It currently builds from source on Linux; downloadable releases are planned.

## What works today

- Standard chess moves and legal-move highlighting, with a promotion picker.
- Check, checkmate, stalemate, castling, en passant, and promotion.
- Automatic draws for fivefold repetition, the 75-move rule, and common insufficient-material positions.
- Local two-person play, a random opponent, and a CPU AI.
- AI search using iterative deepening, negamax, alpha–beta pruning, and a hand-written position evaluator.
- Optional parallel search across a selected number of CPU workers. The default is one worker; Options can select more or use all available logical CPUs.
- Optional **Think during my turn** mode. The AI searches possible replies while the human is deciding on a move.
- Configurable AI depth, thinking time, node limit, worker count, and pondering settings.
- Live move review with Previous, Next, and Return to live controls.
- Saving completed games and replaying them later.
- Board annotations: right-click to mark a square and right-drag to draw an arrow.

The AI does not currently use a trained neural network. More CPU workers can help it finish a deeper search within a time limit, but do not guarantee a stronger move in every position.

Threefold and 50-move draw claims and general dead-position detection remain open.

## Install build requirements on CachyOS or Arch Linux

Open a terminal and run:

```bash
sudo pacman -Syu --needed base-devel cmake git qt6-base qt6-svg
```

Enter your password when prompted. This installs the C++ build tools, CMake, Git, Qt 6 Widgets, and Qt 6 SVG support. You do not need a GPU or Python to build and play the current app.

On a minimal Wayland desktop, you may also need:

```bash
sudo pacman -S --needed qt6-wayland
```

If you already have the project source, you do not need to clone it again. If you are obtaining it through Git, clone its repository first, then enter the downloaded repository directory. The build commands below must run from the directory containing this `README.md` and the `engine/` folder.

## Build, test, and run

From the repository root:

```bash
cmake -S engine -B engine/build-gui \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON &&
cmake --build engine/build-gui --parallel 2 &&
ctest --test-dir engine/build-gui --output-on-failure &&
./chess_gui
```

If your terminal is inside `engine/`, run `cd ..` before the commands. If your terminal is elsewhere, first use `cd` to enter the repository root.

The first command configures the project. The second builds it. The third runs the automated tests. The last starts the app **only if the preceding commands succeed**. To launch it later without rebuilding, run `./chess_gui` from the repository root.

`--parallel 2` sets the number of **compiler jobs during the build**. It does not set the AI's CPU worker count. Change the AI worker count in the app's **Options** menu.

CMake writes temporary build files to `engine/build-gui/`. The executable is built as `chess_gui` beside the `engine/` folder, where it can find the development artwork. Completed-game records are saved under `engine/data/games.tsv`.

If the build succeeds but a test fails, `./chess_gui` will not run as part of the chained command. Read the test output before assuming the executable failed to build.

## Playing and AI options

Choose **Local**, **Random**, or **AI** from the menu. For Random or AI, choose whether to play as White or Black.

In **Options**, you can select the maximum search depth, thinking time in seconds, maximum nodes per AI move, and the number of CPU workers. **Use all** selects all logical CPUs available to the process. **Think during my turn** switches background pondering on or off.

Depth, time, and nodes are limits: the AI stops when it reaches the first applicable limit or finishes a decisive search. The status line shows the last completed depth and the **new** nodes and time spent after your move. It does not include work already done while pondering. The previous AI move's status remains visible while it is your turn.

The current Options choices are held in memory and reset when the app restarts.

## Project layout

| Path | Purpose |
|---|---|
| `engine/include/` and `engine/src/` | Chess rules, game state, AI search, and game saving |
| `engine/gui/main.cpp` | Qt interface, Options, search workers, and pondering control |
| `engine/gui/assets/` | SVG chess pieces |
| `engine/tests/` and `engine/src/ai_test.cpp` | Rules and AI tests in the current source layout |
| `engine/docs/project-notes.md` | Project handoff, known issues, and roadmap |
| `engine/data/games.tsv` | Completed games created locally when you play |

## Planned work

1. Verify the pondering lifecycle, especially using a completed speculative reply when the human moves *after* pondering has reached its depth limit. Measure peak memory with multiple CPU workers.
2. Benchmark one, two, and more workers on the **same chess positions**, recording completed depth, move, score, time, nodes, and memory.
3. Improve the C++ search with measured changes such as quiescence search, a bounded transposition table with Zobrist hashing, Principal Variation Search, and Late Move Reduction.
4. Build a bounded C++ self-play data pipeline and train a compact chess model for move ordering and position evaluation. Keep the current evaluator as a fallback.
5. Package tested, offline Linux and Windows releases that include the required Qt resources and eventual model weights. Evaluate Android later.

The ZergChess design PDF is a source of ideas for future search and machine-learning work. It describes a **different chess-like game**, so its board layout and rules are not specifications for this chess app. This project is proceeding in C++; its Rust implementation is not part of the plan.

For a detailed handoff to another developer or AI agent, see `engine/docs/project-notes.md`.
