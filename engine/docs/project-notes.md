# ZergChess — project handoff

## Refresh the source snapshot first

Run this Bash block from anywhere inside the Git checkout. It writes `engine/docs/project-context.txt` with the current source, tests, build configuration, and documentation. It excludes generated builds, private game data, artwork bytes, and editor swap files.

```bash
(
    set -euo pipefail
    cd "$(git rev-parse --show-toplevel)"
    output=engine/docs/project-context.txt
    mkdir -p engine/docs

    tmp=$(mktemp engine/docs/.project-context.XXXXXX)
    trap 'rm -f "$tmp"' EXIT

    list_files() {
        find engine \
            \( -type d \( -name build -o -name 'build-*' -o -name archive \
                -o -name data -o -name .venv -o -name __pycache__ \
                -o -name .git \) -prune \) -o \
            \( -type f ! -path "$output" \
                ! -name '*.swp' ! -name '*.swo' ! -name '*~' -print0 \) |
            LC_ALL=C sort -z
    }

    {
        printf '# ZergChess source snapshot\n\n## File paths\n'
        list_files | tr '\0' '\n'

        printf '\n## Source, build configuration, and documentation\n'
        list_files | while IFS= read -r -d '' path; do
            case "$path" in
                *.cpp|*.cc|*.c|*.hpp|*.h|*.py|*.sh|*.cmake|\
                */CMakeLists.txt|*.qrc|*.ui|*.md|*.txt|*.json|\
                *.yaml|*.yml|*.toml)
                    printf '\n=== %s ===\n' "$path"
                    cat -- "$path"
                    printf '\n'
                    ;;
            esac
        done
    } > "$tmp"

    mv -- "$tmp" "$output"
    trap - EXIT
    printf 'Wrote %s/%s\n' "$PWD" "$output"
    wc -l -- "$output"
)
```

Before sharing the snapshot, review it for anything private. Attach the refreshed snapshot, the current README, and `ZergChess(1).pdf` to the next agent. If requesting a complete GUI replacement, ensure the snapshot contains the **latest** `engine/gui/main.cpp`. Treat the snapshot as a handoff copy, not a source file to edit.

## Name and working arrangement

The project name is **ZergChess**.

The owner applies changes on their CachyOS computer. For code changes, say which file to change and provide the **entire paste-ready contents of each changed file in chat**. Do not create downloadable code files unless the owner explicitly requests them. Give build and test commands and say clearly when code has not been compiled or tested against the owner's repository.

This handoff describes the last reported state. The live source and fresh test results take precedence.

## Project goal

Build a downloadable Linux desktop chess app using a shared C++ rules engine. Two people can play locally; eventually a person can play against a search-based AI. Completed matches can be saved and replayed. Training happens separately from ordinary gameplay.

The owner develops on a Ryzen 7 9800X3D with 32 GB DDR5 RAM and an RTX 3080. The distributed app must also work on smaller computers **without requiring a GPU**.

## Current state, reported on 8 November 2026

- The Qt 6 Widgets GUI uses SVG pieces. Legal destinations are green with visible borders; the selected square is amber. A checked king's square is red, and all pieces currently checking that king are highlighted faint blue, including double check.
- Local two-person play rotates the **display** after each turn by mapping displayed squares to engine coordinates. It does not rotate or rewrite the underlying engine board.
- The last supplied GUI had a bottom Menu containing Local, Random, disabled future AI, and Replay. Replay offered Local and Random-games sources, Game ID entry, and previous/next move controls. **Inspect the current GUI source** before assuming all of these remain. The owner has since removed the headless random-data generator, so the Random-games replay source may now be unnecessary.
- `Game` owns turn order, move history, result, and saving. `Board` owns chess rules and legality. The shared engine supports ordinary moves, castling, en passant, promotion, checkmate, and stalemate. Automatic draws cover fivefold repetition, the 75-move rule, and several common insufficient-material cases.
- Threefold and 50-move claims, general dead-position detection, stronger rule tests, and release packaging remain open.
- Completed local games save to `engine/data/games.tsv`. The reported format has `game_id`, `result`, `reason`, `moves`, and `final_placement`. Moves use coordinate notation such as `e2e4` and `a7a8q`. `final_placement` is the piece-placement portion of FEN, not a complete FEN. Older four-column history was designed to migrate to IDs with a backup. Confirm the exact current implementation before changing storage.
- The standalone random-vs-random batch generator was **removed at the owner's request**. Its role is now a randomized **rules regression test**, `engine/tests/random_rules_test.cpp`. It plays 20 seeded games in memory, up to 300 plies, checks accepted moves, and replays the move sequences. It does not write game data. The owner showed that `engine/data/random_games.tsv` is absent.
- The owner's last Debug configure/build succeeded. CTest passed **4/4**: `castling_test`, `draw_test`, `move_count_test`, and `random_rules_test`. The random test took about 1.71 seconds on their computer. A Qt message about optional `WrapVulkanHeaders` did not block the build.
- `engine/build/` still contains old binaries, including an obsolete terminal `chess` executable. That is a stale build directory, not proof that the terminal target remains in current CMake. The active build directory was `engine/build-gui`; CMake placed `chess_gui` in the repository root.
- The randomized test runs sequentially. `cmake --build ... --parallel` affects compilation, not the test's number of CPU threads.

## ZergChess PDF: intended search

The brother's PDF describes a **neural-network-enhanced negamax** chess engine. It is a design reference from another implementation, not proof that its performance or training approach already works in this repository.

### Negamax and alpha–beta

For each legal move, search the resulting position from the opponent's perspective, negate that score, and choose the maximum. Checkmate and other terminal results take precedence over a static or learned position evaluation.

Alpha–beta search avoids examining branches that cannot improve the decision. Its effectiveness depends heavily on trying promising moves early. The C++ chess engine remains the authority for legal moves; a neural-network legality prediction cannot authorize a move.

### Move ordering and search optimizations

1. **Transposition table:** use a Zobrist key to identify previously searched positions. The key must represent all relevant chess state: pieces, side to move, castling rights, and en-passant state. Store depth, score, bound type (exact, lower, or upper), and a suggested best move. Try that move first. Account carefully for draw context and mate-score distance.
2. **Network ordering:** after the table move, order other legal moves using network predictions. This is a search priority, not a final verdict on which move is best.
3. **Principal Variation Search (PVS):** thoroughly search the most promising move; probe later moves using a narrow window, then re-search a move if the probe suggests it could become best.
4. **Late Move Reduction (LMR):** search later quiet moves at a reduced depth first, re-searching at full depth if necessary. Checks, tactical moves, shallow positions, and principal variations need safeguards.
5. **Quiescence search:** at the normal depth boundary, continue through relevant tactical moves rather than evaluating a position midway through an exchange. The PDF mentions captures and promotions. If the side to move is in check, search legal evasions; do not accept a static “stand pat” evaluation while still in check.

The PDF mentions a transposition table of around one million entries and an NN cache. These are details of that implementation, **not required defaults** for the shipped ZergChess app.

### Proposed neural-network structure

- Represent the chessboard as square nodes with piece information, coordinates, side to move, pawn identity where needed for the proposed action slots, and relevant state such as en passant.
- Directional edges in eight compass directions provide line-of-sight information, including the direction, distance to the first obstacle, and obstacle type.
- The described network uses a node encoder, six residual edge-gated message-passing blocks, global pooling/readout, and a slot-wise policy head. The PDF suggests width 512 and 15% training dropout. These are starting design choices rather than proven requirements for the target runtime budget.
- The PDF text says `8x5` for the square grid in one place. That conflicts with a normal 8×8 chessboard. Clarify this before implementing the graph layout.
- The value head predicts a position value and learned uncertainty; the PDF proposes Student-t negative log-likelihood against eventual game outcomes.
- Policy move slots predict auxiliary legality, how likely a legal move is to be fully examined by search, and a value for examined moves. A pairwise ranking loss teaches the network which moves to search first. The PDF describes combining visit probability and move value with a 0.5 mixing weight. Define the exact slots, promotion choices, castling, and player perspective before implementation.

### Training data and GPU batching

The intended teaching data comes from **search**, including the position, candidate legal moves, which candidates were searched sufficiently, their scores or bounds, and eventual outcomes. The current local-game TSV only stores the played sequence, result, and final placement. It cannot by itself provide the PDF's search-policy targets.

Split training and validation by **entire game**, never by individual positions from the same game. A simulation stopped by a move limit is `unfinished`, not a draw.

The PDF describes a separate Rust prototype that pauses searches when they request neural-network inference, batches requests on the GPU, and resumes the searches. This is a later training/data-generation optimization. It is not required to make the distributed desktop app playable. Its random opening exploration and initial simple evaluator also do not require bringing back the deleted random batch generator.

## Resource and distribution requirements

The owner's proposed default is **2 CPU workers** and a **2 GB engine-managed memory budget**, adjustable in Options, with a “Maximum performance” preset. The memory setting can bound structures the program owns, such as a transposition table or NN cache; it must not be advertised as a guaranteed cap on total process memory.

Add an adjustable thinking-time limit per AI move so complex positions cannot search indefinitely. Training may use the owner's GPU and larger resource budgets as a separate developer process. Playing and launching the shipped app must work without a GPU.

Do not add two search workers merely to satisfy the setting: a first single-thread alpha–beta search is a reasonable starting point. Implement and test search parallelism when it has a clear design and measurable benefit. Benchmark speed, peak memory, and move quality on modest CPU-only hardware before claiming broad compatibility.

## Next implementation milestone

1. Obtain a freshly generated source snapshot and current README. Check whether the GUI still exposes a Random-games replay source even though the random dataset generator has been removed. Do not recreate random datasets without a new request.
2. Build a **correct, bounded CPU search** using fixed-depth negamax, terminal scores, simple material evaluation, and legal moves from the existing engine. Add deterministic tactical tests and record nodes searched and elapsed time.
3. Check how search positions can be copied or undone safely. The board owns pieces through `unique_ptr`; do not assume it has a working copy constructor. Do not mutate the live GUI game during search. Preserve castling, en passant, promotion, repetition, and draw state.
4. Add alpha–beta pruning and compare its node count with plain negamax. Then add move ordering, a memory-budgeted transposition table, and check-aware quiescence. Add PVS and LMR only after establishing correctness and benchmarks for the simpler search.
5. Integrate AI play into the GUI on a bounded worker task so the window remains responsive. Expose the agreed resource controls. Add the neural evaluator and search-generated training examples after the CPU search is trustworthy.

## Build and verification

From the repository root:

```bash
cmake -S engine -B engine/build-gui -DCMAKE_BUILD_TYPE=Debug
cmake --build engine/build-gui --parallel
ctest --test-dir engine/build-gui --output-on-failure
./chess_gui
```

Refresh this handoff and the source snapshot after each substantial milestone. Always use current source and fresh tests to decide what is implemented.
