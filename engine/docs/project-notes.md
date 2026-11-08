# ZergChess — project handoff

## Working arrangement

The owner develops on CachyOS and applies code changes locally. Provide complete, copy-and-paste file contents when changing a source file; avoid a long sequence of small replacements. State exactly which path each block belongs to. Do not create downloadable files unless requested. Distinguish tests run by the owner from tests run by the assistant.

The repository root is the directory containing `engine/`. Run the build commands from there. If the terminal is inside `engine/`, run `cd ..` first.

## Collect the current code for the next agent

Run this Bash block from anywhere inside the Git checkout. It prints the current source, tests, build configuration, and documentation as one text stream. It includes uncommitted files and skips build output, saved games, datasets, model weights, and common private files. **It does not create a folder or save a snapshot.**

The output may be long. Copy it from the terminal into the next conversation, along with this handoff and the design PDF. Review it first because private text can still appear in ordinary source files.

```bash
(
    set -euo pipefail
    cd /mnt/4tb_games/Projects/Chess_Machine/Chess-Engine

    output=engine/docs/project-context.txt
    mkdir -p engine/docs

    tmp=$(mktemp engine/docs/.project-context.XXXXXX)
    trap 'rm -f -- "$tmp"' EXIT

    list_files() {
        find . \
            \( -type d \( \
                -name .git -o -name .venv -o -name __pycache__ \
                -o -name node_modules -o -name build -o -name 'build-*' \
                -o -name 'cmake-build-*' -o -name CMakeFiles \
                -o -name archive -o -name dist -o -name data \
                -o -name datasets -o -name checkpoints -o -name runs \
            \) -prune \) -o \
            \( -type f \
                ! -path "./$output" \
                ! -name '.project-context.*' \
                ! -name '.env' ! -name '.env.*' \
                ! -name '*.pem' ! -name '*.key' \
                ! -name '*.swp' ! -name '*.swo' ! -name '*~' \
                ! -name '*.tsv' ! -name '*.pgn' ! -name '*.jsonl' \
                ! -name '*.pt' ! -name '*.pth' ! -name '*.onnx' \
                ! -name '*.o' ! -name '*.a' ! -name '*.so' \
                ! -name '*.dll' ! -name '*.exe' ! -name chess_gui \
                -print0 \) |
            LC_ALL=C sort -z
    }

    {
        printf '# ZergChess source snapshot\n\n## File paths\n'
        list_files | tr '\0' '\n'

        printf '\n## Source, build configuration, and documentation\n'
        while IFS= read -r -d '' path; do
            case "$path" in
                */CMakeLists.txt|*/Dockerfile|*/Makefile|*/LICENSE*|*/COPYING*|\
                *.c|*.cc|*.cpp|*.cxx|*.h|*.hh|*.hpp|*.hxx|*.inl|\
                *.py|*.sh|*.bash|*.ps1|*.bat|*.cmake|*.qrc|*.ui|\
                *.qml|*.java|*.kt|*.gradle|*.xml|*.desktop|*.iss|\
                *.md|*.txt|*.json|*.yaml|*.yml|*.toml|*.ini)
                    printf '\n=== %s ===\n' "${path#./}"
                    cat -- "$path"
                    printf '\n'
                    ;;
            esac
        done < <(list_files)
    } > "$tmp"

    mv -- "$tmp" "$output"
    trap - EXIT
    printf 'Done: %s/%s\n' "$PWD" "$output"
)
```

This is a reference snapshot. The next agent should make changes in the actual source files.
```

**Important:** This script replaces the previous `engine/docs/project-context/` snapshot when run again. It does not modify the actual source files.

For a single attachment, run this afterward from the repository root:

```bash
tar -czf zergchess-project-context.tar.gz \
    -C engine/docs project-context
```

Attach `zergchess-project-context.tar.gz`, this handoff, the current README, and the ZergChess PDF to the next conversation. If a build fails, also provide the command and complete compiler or test output.

## Project and design scope

ZergChess is a chess app with a shared C++17 rules engine and a Qt 6 Widgets GUI. The C++ engine determines chess legality and game results.

The supplied ZergChess PDF describes a **different game with some chess-like rules**. It was a homework design reference, not a specification for this app's rules. Adapt its useful search and neural-network ideas to standard chess; do not import its board dimensions, piece rules, move slots, or game outcomes literally. In particular, the PDF's `8×5` reference is for the other game and should not be treated as a chessboard dimension.

Ignore the PDF's Rust implementation. This project will proceed in C++. Its pausable, batched self-play idea may be reconsidered as a C++ design later, but Rust is not a project requirement.

## Current implementation — 9 November 2026

- The GUI offers Local, Random, AI, Replay, and Options. Local play flips the board with the turn; AI and Random games retain the human player's chosen orientation.
- `Board` implements chess legality. `Game` handles turns, move history, results, and saving. The engine supports castling, en passant, promotion, checkmate, stalemate, fivefold repetition, the 75-move rule, and common insufficient-material draws. Threefold and 50-move claims and general dead-position detection remain open.
- `engine/src/ai.cpp` implements iterative deepening, negamax, alpha–beta pruning, basic move ordering, and a hand-written material and placement evaluator. There is no neural model, transposition table, Zobrist hashing, opening book, or endgame database.
- AI search runs away from the GUI thread. Root candidates can be distributed across CPU workers. A single search shares its wall-clock deadline and total node budget across those workers. Each candidate gets an independent mutable `Game` copy. The default worker count is one; the Options page detects the available logical CPUs and offers a worker count up to “Use all.”
- “Think during my turn” enables optional background pondering. It searches AI replies to the human's legal moves in increasing depth rounds and stores completed move/score/depth records rather than entire search trees. Turning it off was confirmed by the owner to stop the large amount of background computation.
- Normal AI search and pondering now have a depth relationship: a pondered result deeper than the normal selected depth must not be reused, and pondering must stop at the smaller of the normal depth limit and `ponder_max_depth`. The owner reported the latest version working after adding `ponder_max_depth` to `SearchLimits`. **Verify the exact current Options control and depth behavior from the refreshed source snapshot**; the conversation contained several rounds of manual edits.
- The Options time control displays seconds and converts to milliseconds internally. The owner set a selectable maximum of 3,600 seconds, normal depth maximum of 12, and node maximum of 1,000,000,000. The backend `choose_move()` clamps time to 3,600,000 ms and nodes to 1,000,000,000. Search stops when the first depth, time, node, cancellation, or solved-position condition is reached. These are ceilings, not targets.
- The status line reports the last completed AI depth, **new nodes on the AI's turn**, and elapsed time on that turn. It excludes previous pondering work and remains visible during the following human turn. Consequently, status figures alone do not measure total work or establish move quality.
- The board supports square marks and arrows. Live review navigates positions from the current game and pauses AI play while reviewing. Saved replay reconstructs positions from recorded moves.
- Finished games are saved under `engine/data/games.tsv` in the development build.

### Verification reported by the owner

- The Release GUI and all test targets built on CachyOS after the parallel-search changes.
- Four rules tests passed. The original `ai_test` failed on a rejected `g1f3` in its queen-capture setup. A corrected complete AI test was supplied afterward, but **a passing run of the revised five-test suite was not shown**. Do not claim 5/5 passed without rerunning it.
- The owner observed 16 CPU workers becoming active. In informal games using roughly ten seconds per AI turn, 16 workers often reported completed depth 6 and once depth 7; one worker often reported depth 5 and once depth 6. These were not matched positions and do not establish a speedup or playing-strength result.
- With pondering enabled, displayed “new nodes” excluded speculative work done during the human turn. With pondering disabled, the owner saw much smaller new-node and elapsed-time readings. This is expected from the meaning of the counter.
- The earlier whole-process observation of about 30–31 MiB predates this parallel and pondering work. **Current peak memory with 16 workers has not been measured.**

### Known issue to inspect before adding more search features

In the supplied GUI implementation, if pondering finishes its full depth rounds **before the human moves**, `tick()` can consume and discard the vector of completed replies. A later human move would then miss the result. Confirm whether this still exists in the refreshed `engine/gui/main.cpp`. If so, retain the small completed reply vector until the human moves; then select its matching entry and release all others. Cancel and clear it on a new game, mode change, review, options change, or shutdown. Do not retain full `Game` trees for every possible human move.

Also check that the selected normal depth remains an absolute upper bound in both modes, and that `ponder_max_depth` is declared in `engine/include/ai.hpp`, displayed or intentionally fixed in Options, and used in `ponder()`.

## Next engineering work

1. **Finish and verify pondering lifecycle.** Test the human moving immediately, moving after pondering reaches its cap, promotion, a terminal move, mode changes, review, closing the GUI, and changing Options. Confirm that only the matching completed reply survives the human move. Measure peak resident memory with one and many workers.
2. **Benchmark parallel search on identical chess positions.** Record position, selected move, score, completed depth, nodes, wall time, and peak memory. Compare one, two, and higher worker counts at fixed completed depth and at fixed wall time. More nodes with more workers do not necessarily mean a stronger move; parallel branches can lose pruning opportunities. Preserve deterministic tie-breaking where possible.
3. **Add search refinements one at a time, with tests and measurements.** The PDF suggests quiescence search, a bounded transposition table with Zobrist hashing, Principal Variation Search, and Late Move Reduction. Adapt all of them to standard chess. A transposition table needs correct side-to-move, castling, en-passant, repetition/draw context, bound type, and mate-score handling. Keep its memory budget explicit.
4. **Build a bounded, headless C++ self-play and training-data exporter.** Record chess positions, legal candidate moves, completed search depth, scores and score-bound type, and eventual outcomes. Stream data rather than retaining whole runs in RAM; split training and validation by whole game.
5. **Train and integrate a compact chess model.** Start with learned move ordering and a value head, benchmark CPU inference and memory, and keep the hand-written evaluator as a fallback. The PDF's graph network, auxiliary heads, uncertainty model, and large NN cache are ideas to evaluate later, not current requirements. Do not assume a 120 GB cache is suitable for the shipped app.
6. **Package Linux and Windows releases.** Include Qt dependencies, SVG assets, and any eventual model weights. Move saves/settings to a platform-appropriate user directory, then test clean offline installs. Consider Android only after desktop packaging.

## Build and verify

Run from the repository root:

```bash
cmake -S engine -B engine/build-gui \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON &&
cmake --build engine/build-gui --parallel 2 &&
ctest --test-dir engine/build-gui --output-on-failure &&
./chess_gui
```

`cmake --build ... --parallel 2` controls **compiler jobs**, not the in-game CPU worker count. If `ctest` fails, the chained command does not launch the GUI. The GUI executable may still have built successfully; inspect the test failure before changing chess rules.

Options are currently in memory and reset to defaults when the app restarts unless settings persistence is added later.
