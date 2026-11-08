# Chess Engine — project notes and handoff

Run this from anywhere inside the Git checkout to refresh `engine/docs/project-context.txt`:

```bash
(
    set -euo pipefail
    cd "$(git rev-parse --show-toplevel)"
    output=engine/docs/project-context.txt

    list_files() {
        find engine \
            \( -type d \( -name build -o -name archive -o -name data \
                -o -name .venv -o -name __pycache__ \) -prune \) -o \
            \( -type f ! -path "$output" -print0 \) |
            LC_ALL=C sort -z
    }

    {
        printf '# Chess Engine project snapshot\n\n## File paths\n'
        list_files | tr '\0' '\n'

        printf '\n## Source, build configuration, and documentation\n'
        list_files | while IFS= read -r -d '' path; do
            case "$path" in
                *.cpp|*.cc|*.c|*.hpp|*.h|*.py|*.sh|*.cmake|\
                */CMakeLists.txt|*.qrc|*.ui|*.md|*.txt|*.json|\
                *.yaml|*.yml|*.toml|*/LICENSE*|*/COPYING*)
                    printf '\n=== %s ===\n' "$path"
                    cat -- "$path"
                    printf '\n'
                    ;;
            esac
        done
    } > "$output"

    printf 'Wrote %s/%s\n' "$PWD" "$output"
    wc -l -- "$output"
)
```

Attach the generated snapshot and `engine/README.md` to a fresh chat. The snapshot includes these notes, source files, tests, and CMake configuration. Artwork paths are listed without dumping the artwork. Build outputs and personal match history are excluded. Refresh the snapshot after meaningful changes. Treat it as a handoff copy rather than a file to edit.

## Project goal

Build a downloadable Linux desktop chess app where two people can play on the same computer, or one person can play against a random opponent or my own AI. Use the same C++ rules engine for every mode. Save completed games so players can browse and replay their matches. Make those records usable by a separate Python pipeline that can also import public chess games for training.

This is a learning project in C++, desktop software, and machine learning. I want a working app I can share on GitHub, a clear explanation of how it works, and evidence that the engine and data pipeline behave as intended.

## Current state

- The **Qt 6 desktop app works locally on CachyOS**. Two people can play on one computer. The board uses SVG pieces, highlights legal destinations, prompts for promotion, and shows a result popup.
- The C++ `Game` class handles turn order, legal moves, the result, and game saving. It uses `Board` for chess rules. Keep the GUI, random player, and future AI behind this shared interface.
- Ordinary moves, checkmate, stalemate, promotion, en passant, and castling are implemented. Automatic draws cover fivefold repetition, the 75-move rule, and common insufficient-material positions.
- Tests have passed for castling, insufficient material and board reset, and the 75-move threshold and counter resets. Manual runs have checked Fool’s Mate, promotion, en passant expiry, and game saving. These checks do not establish complete rule coverage.
- The terminal front end has been removed from the project layout. **Verify `engine/CMakeLists.txt` no longer references `cli/main.cpp`**, then configure from scratch. An old executable in `engine/build/` only proves an earlier build worked.
- Completed games append to `engine/data/games.tsv`. The popup shows the save path. Closing or discarding an unfinished local game does not save it.
- The current TSV has `result`, `reason`, `moves`, and `final_placement`. Moves use coordinate notation such as `e2e4` and `b7a8n`. The final placement is the piece-placement field of FEN. The move list can reconstruct a game from the standard starting position.
- Still open: threefold and 50-move draw claims, general dead-position detection, broader rule verification, match history and replay, bots, dataset import and export, and release packaging.

## Where things live

| Path | Purpose |
|---|---|
| `chess_gui` | Locally built executable, placed beside `engine/` after a build. |
| `engine/src/` and `engine/include/` | Shared C++ rules and game interface. |
| `engine/gui/` and `engine/gui/assets/` | Desktop app and SVG pieces. |
| `engine/tests/` | Automated checks for the shared rules. |
| `engine/data/` | Local game records. Keep personal history out of Git. |
| `engine/docs/` | Project notes and generated handoff snapshot. |
| `engine/build/` | Generated CMake files and compiled outputs. Recreate these on each computer. |
| `engine/CMakeLists.txt` | Build recipe for the shared library, desktop app, and tests. |
| `engine/README.md` | Build and launch instructions for a new computer. |

## Plan forward

### 1. Finish the desktop-only build and strengthen the rules tests

- Remove any remaining CLI target or CLI instructions. Build in a fresh build directory to catch stale dependencies. Run the automated tests, launch the GUI, and finish a game to verify artwork, the result popup, and the saved record.
- Add a legal move-count test that explores several moves deep and compares the totals with known reference positions. Include targeted tests for en passant exposing your own king, promotion as the only legal move, and a known stalemate. Keep these tests independent of the GUI.
- Add threefold and 50-move draw claims through `Game` and the GUI. Cover claims based on the current position and on a specified legal move. Test accepted and rejected claims and confirm that an invalid claim does not advance the turn.
- Keep the current dead-position detection documented as limited. Stalemate means the player has no legal move and is not in check. A dead position can still have legal moves, but neither side can ever deliver mate. A simulation hitting its run limit is `unfinished`, never a chess draw.

**Done when:** a fresh desktop-only build works, the tests pass, and the README accurately describes the supported rules and remaining limitations.

### 2. Give saved games a stable format and build match history

- Preserve the existing TSV records. Add a reader for the old format before changing the writer or migrating any history.
- Define a versioned game record with a game ID, timestamp, White and Black player types, source, starting position, ordered moves, result, and termination reason. Keep result and reason separate so a resignation can be recorded even if the board still has legal moves.
- Move record reading and writing into shared code so local play, bots, and imported games can use the same model. Decide the storage format after checking that it can round-trip both current TSV records and imported games with extra metadata.
- Add a **History** view in the desktop app. Start with a list showing who played, when, and what happened. Add replay with previous and next move controls using a separate board so browsing history cannot affect the current game.

**Done when:** old games still open, new games are saved once, and a saved game can be replayed from start to finish.

### 3. Add a random opponent and a batch runner

- Expose all legal moves for the side to move through `Game`. Sample uniformly from that list for the first random opponent. Use the same interface for local play and automated games.
- Add a mode picker for **Human vs Human** and **Human vs Random**. Keep the GUI responsive while a bot is choosing. Use a seed for repeatable test runs.
- Build a separate batch runner for random vs random. This is a data generation tool, so it does not need the old interactive terminal front end or a GUI. Give it a game limit, a move limit per game, a seed, and an output path.
- Label the source of each game. If a game reaches the move limit, record `unfinished` separately from completed results. Keep batch output separate from personal match history by default.

**Done when:** a person can play the random bot in the app, and a seeded batch run produces replayable records with correct results.

### 4. Import public games and build the Python data pipeline

- Start with a small sample of standard-chess PGN games. Use a PGN parser to read the metadata and replay the moves, then translate them into the same game record used by the C++ app.
- Preserve source IDs and provenance, recorded outcomes, and available termination reasons. Check the dataset’s license before redistributing anything. Report skipped or invalid games so import problems are visible.
- Export training examples by replaying each game. Start with the position before each move, the legal moves, the move played, the result from the moving player’s perspective, and the source game ID.
- Keep all positions from one game in the same train, validation, or test split. Keep human, random, and AI sources distinguishable so experiments can select or compare them independently.

**Done when:** one local game and one imported PGN game can both be replayed and exported through the same pipeline, with valid positions, moves, and labels.

### 5. Train a baseline AI and connect it to the app

- Keep the random bot as a baseline. For a first model, try predicting human moves or learning position values from game results. Random self-play is useful for generating positions and testing the pipeline, but random moves are poor targets for learning which move to choose.
- If experimenting with rewards for captures, measure the effect separately. A capture reward can encourage the model to take material at the expense of king safety or winning. Judge progress using game outcomes.
- Give the Python engine a small interface that receives a position and returns a move. The C++ game remains responsible for validating the answer. Handle an invalid move, timeout, or engine crash without freezing the window.
- Compare versions in repeatable matches with colors swapped. Record wins, draws, losses, and illegal responses against the same opponents and settings. Use those results to decide whether AI self-play is improving the model.

**Done when:** I can select my AI in the desktop app, play a complete game, and compare it with the random baseline using recorded results.

### 6. Publish a downloadable Linux release

- Keep source-build instructions in the README and record the compiler, CMake, Qt, and Linux setup actually tested. Include a screenshot and clear build and launch steps.
- Package the desktop executable, artwork, and required runtime components for the target system. Decide where the packaged app saves user history so it can write there even when installed in a read-only location. Verify artwork redistribution rights.
- Test the release on another machine or a clean environment. Launch it without building the source, complete a game, and confirm that the saved history is available after restarting the app.

**Done when:** someone can follow the GitHub link, download the release, open the app, and play and save a game without setting up a compiler. This release can ship before the AI is ready.

## How to pick up the work

Start with the actual snapshot and build instructions. These notes describe intent and recorded progress. The current source and fresh test results determine what is implemented. For code changes, provide commented, paste-ready files or complete sections and one build-and-test pass per batch. The project owner applies changes locally.

**Next action:** verify the CLI target is gone from CMake, update the desktop-only README if it still mentions terminal play, then do a clean configure, build, and test. After that, add the first broader rules regression tests. Update this paragraph after each milestone with what changed, which checks passed, and the next concrete task.
