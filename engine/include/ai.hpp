#pragma once

#include "game.hpp"

#include <atomic>
#include <cstdint>
#include <optional>
#include <vector>

namespace zerg {

struct SearchLimits {
    int max_depth = 3;
    int time_ms = 500;
    std::uint64_t max_nodes = 20000;
    bool alpha_beta = true;
    int workers = 1;
    int ponder_max_depth = 6;
};

struct SearchResult {
    std::optional<GameMove> move;
    int score_cp = 0;
    int completed_depth = 0;
    std::uint64_t nodes = 0;
    std::int64_t elapsed_ms = 0;
    bool interrupted = false;
};

// Evaluators return centipawns from the side-to-move perspective.
// Evaluators must be thread-safe.
using Evaluator = int (*)(const Game&);

int evaluate(const Game& game);

// Detected once on the first call.
// Call during application startup to populate the worker selector.
int available_workers();

// A seed must belong to this exact position and use the same evaluator
// and search settings. Only completed pondering results should be supplied.
SearchResult choose_move(
    const Game& position,
    SearchLimits limits = {},
    const std::atomic_bool* cancel = nullptr,
    Evaluator evaluator = evaluate,
    const SearchResult* seed = nullptr);

struct PonderReply {
    GameMove human_move;
    SearchResult reply;
};

// Searches possible human moves in increasing depth rounds.
// Stores one completed AI reply per human move, not entire search trees.
// A cancellation flag is required.
std::vector<PonderReply> ponder(
    const Game& position,
    SearchLimits limits,
    const std::atomic_bool* cancel,
    Evaluator evaluator = evaluate);

} // namespace zerg