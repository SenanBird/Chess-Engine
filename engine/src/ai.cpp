#include "ai.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <sched.h>
#endif

namespace zerg {

int available_workers() {
    static const int detected = [] {
#if defined(__linux__)
        cpu_set_t allowed;
        CPU_ZERO(&allowed);

        if (sched_getaffinity(0, sizeof(allowed), &allowed) == 0) {
            const int count = CPU_COUNT(&allowed);
            if (count > 0) return count;
        }
#endif

        const unsigned count = std::thread::hardware_concurrency();
        return count == 0 ? 1 : static_cast<int>(count);
    }();

    return detected;
}

namespace {

constexpr int infinity = 32000;
constexpr int mate_score = 30000;
constexpr int evaluation_limit = 20000;
constexpr int maximum_depth = 64;

using Clock = std::chrono::steady_clock;

int piece_value(const Piece* piece) {
    if (!piece) return 0;
    if (piece->name == "Pawn") return 100;
    if (piece->name == "Knight") return 320;
    if (piece->name == "Bishop") return 330;
    if (piece->name == "Rook") return 500;
    if (piece->name == "Queen") return 900;
    if (piece->name == "King") return 20000;
    return 0;
}

int promotion_value(char promotion) {
    switch (promotion) {
        case 'q': return 900;
        case 'r': return 500;
        case 'b': return 330;
        case 'n': return 320;
        default: return 0;
    }
}

bool same_move(const GameMove& a, const GameMove& b) {
    return a.from_row == b.from_row &&
           a.from_col == b.from_col &&
           a.to_row == b.to_row &&
           a.to_col == b.to_col &&
           a.promotion == b.promotion;
}

int ordering_score(const Game& game, const GameMove& move) {
    const Board& board = game.board();

    const Piece* attacker =
        board.get_piece(move.from_row, move.from_col, board);
    const Piece* victim =
        board.get_piece(move.to_row, move.to_col, board);

    int captured = piece_value(victim);

    // A legal diagonal pawn move to an empty square is en passant.
    if (!victim && attacker && attacker->name == "Pawn" &&
        move.from_col != move.to_col) {
        captured = 100;
    }

    int score = promotion_value(move.promotion) * 16;

    if (captured != 0) {
        score += 10000 + captured * 16 - piece_value(attacker);
    }

    return score;
}

void order_moves(
    const Game& game,
    std::vector<GameMove>& moves,
    const std::optional<GameMove>& preferred = std::nullopt) {

    std::stable_sort(
        moves.begin(), moves.end(),
        [&](const GameMove& a, const GameMove& b) {
            const int a_score =
                preferred && same_move(a, *preferred)
                    ? 1000000
                    : ordering_score(game, a);

            const int b_score =
                preferred && same_move(b, *preferred)
                    ? 1000000
                    : ordering_score(game, b);

            return a_score > b_score;
        });
}

int terminal_score(const Game& game, int ply) {
    if (game.result() == "1/2-1/2") return 0;

    const bool white_won = game.result() == "1-0";

    return white_won == game.white_to_move()
        ? mate_score - ply
        : -mate_score + ply;
}

bool solved_result(const SearchResult& result) {
    return result.completed_depth > 0 &&
           std::abs(result.score_cp) >= mate_score - 128;
}

struct Interrupted {};

class Search {
public:
    SearchLimits limits;
    const std::atomic_bool* cancel;
    Evaluator evaluator;

    Clock::time_point started = Clock::now();
    Clock::time_point deadline;

    std::atomic<std::uint64_t> nodes{0};
    std::atomic_bool aborted{false};

    Search(
        SearchLimits requested,
        const std::atomic_bool* cancellation,
        Evaluator evaluation)
        : limits(requested),
          cancel(cancellation),
          evaluator(evaluation),
          deadline(
              started +
              std::chrono::milliseconds(requested.time_ms)) {}

    void check_time() const {
        if (aborted.load(std::memory_order_relaxed) ||
            (cancel && cancel->load(std::memory_order_relaxed)) ||
            (limits.time_ms > 0 && Clock::now() >= deadline)) {
            throw Interrupted{};
        }
    }

    void enter_node() {
        check_time();

        // Zero is used internally for unlimited pondering.
        // Normal move searches always have a positive node limit.
        if (limits.max_nodes == 0) {
            nodes.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        auto current = nodes.load(std::memory_order_relaxed);

        for (;;) {
            if (current >= limits.max_nodes) {
                throw Interrupted{};
            }

            if (nodes.compare_exchange_weak(
                    current,
                    current + 1,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed)) {
                return;
            }

            check_time();
        }
    }

    std::vector<GameMove> legal_moves(
        Game& game,
        bool bounded = true) {

        std::vector<GameMove> moves;

        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                if (bounded) check_time();

                auto from = game.legal_moves_from(row, col);
                moves.insert(moves.end(), from.begin(), from.end());
            }
        }

        return moves;
    }

    int negamax(
        Game& game,
        int depth,
        int alpha,
        int beta,
        int ply) {

        enter_node();

        if (game.finished()) {
            return terminal_score(game, ply);
        }

        if (depth == 0) {
            return std::clamp(
                evaluator(game),
                -evaluation_limit,
                evaluation_limit);
        }

        auto moves = legal_moves(game);

        if (moves.empty()) {
            throw std::logic_error(
                "Unfinished game has no legal moves.");
        }

        order_moves(game, moves);

        int best = -infinity;

        for (const auto& move : moves) {
            check_time();

            Game child = game;

            if (!child.play(move)) {
                throw std::logic_error(
                    "Engine rejected a generated move.");
            }

            const int score = limits.alpha_beta
                ? -negamax(
                      child, depth - 1, -beta, -alpha, ply + 1)
                : -negamax(
                      child, depth - 1,
                      -infinity, infinity, ply + 1);

            best = std::max(best, score);
            alpha = std::max(alpha, score);

            if (limits.alpha_beta && alpha >= beta) {
                break;
            }
        }

        return best;
    }

    std::pair<GameMove, int> root(
        Game& game,
        std::vector<GameMove> moves,
        int depth,
        const std::optional<GameMove>& preferred) {

        enter_node();

        if (moves.empty()) {
            throw std::logic_error(
                "Cannot search a root without legal moves.");
        }

        order_moves(game, moves, preferred);

        auto search_candidate = [&](std::size_t index, int alpha) {
            check_time();

            // Every task owns its mutable board and game history.
            // The shared root is only read while tasks are active.
            Game child = game;

            if (!child.play(moves[index])) {
                throw std::logic_error(
                    "Engine rejected a root move.");
            }

            return limits.alpha_beta
                ? -negamax(
                      child, depth - 1, -infinity, -alpha, 1)
                : -negamax(
                      child, depth - 1, -infinity, infinity, 1);
        };

        // Search the leading candidate first to establish a bound.
        const int first_score = search_candidate(0, -infinity);

        GameMove best_move = moves.front();
        int best = first_score;

        const int count = std::min(
            std::max(1, limits.workers),
            static_cast<int>(moves.size()));

        // Preserve serial alpha-beta behaviour with one worker.
        if (count == 1) {
            int alpha = first_score;

            for (std::size_t i = 1; i < moves.size(); ++i) {
                const int score = search_candidate(i, alpha);

                if (score > best) {
                    best = score;
                    best_move = moves[i];
                }

                alpha = std::max(alpha, score);
            }

            check_time();
            return {best_move, best};
        }

        std::vector<int> scores(moves.size(), -infinity);
        scores[0] = first_score;

        std::atomic<std::size_t> next{1};
        std::mutex error_mutex;
        std::exception_ptr error;

        auto record_error = [&](std::exception_ptr failure) {
            std::lock_guard<std::mutex> lock(error_mutex);

            if (!error) {
                error = failure;
            }

            aborted.store(true, std::memory_order_relaxed);
        };

        auto run = [&] {
            try {
                for (;;) {
                    check_time();

                    const auto index =
                        next.fetch_add(1, std::memory_order_relaxed);

                    if (index >= moves.size()) {
                        return;
                    }

                    // Each worker writes to its own score element.
                    //
                    // Candidates beating first_score get exact scores.
                    // Other candidates only need to establish that they
                    // cannot beat the first candidate.
                    scores[index] =
                        search_candidate(index, first_score);
                }
            } catch (...) {
                record_error(std::current_exception());
            }
        };

        std::vector<std::future<void>> jobs;
        jobs.reserve(static_cast<std::size_t>(count - 1));

        try {
            for (int i = 1; i < count; ++i) {
                jobs.push_back(
                    std::async(std::launch::async, run));
            }

            // The calling search thread participates, so count is the
            // total number of active search workers.
            run();
        } catch (...) {
            record_error(std::current_exception());
        }

        // Join every worker before destroying anything it references.
        for (auto& job : jobs) {
            job.get();
        }

        if (error) {
            std::rethrow_exception(error);
        }

        check_time();

        // Tie-breaking follows move order, not thread completion order.
        for (std::size_t i = 1; i < moves.size(); ++i) {
            if (scores[i] > best) {
                best = scores[i];
                best_move = moves[i];
            }
        }

        return {best_move, best};
    }
};

} // namespace

int evaluate(const Game& game) {
    const Board& board = game.board();

    int white_score = 0;
    int white_bishops = 0;
    int black_bishops = 0;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            const Piece* piece = board.get_piece(row, col, board);
            if (!piece) continue;

            const int relative_row =
                piece->is_white ? 7 - row : row;

            // Zero at corners, six in the central four squares.
            const int center =
                6 - (std::abs(2 * col - 7) +
                     std::abs(2 * row - 7)) / 2;

            int value =
                piece->name == "King" ? 0 : piece_value(piece);

            if (piece->name == "Pawn") {
                value +=
                    std::max(0, relative_row - 1) * 8 + center * 2;
            } else if (piece->name == "Knight") {
                value += center * 10;
            } else if (piece->name == "Bishop") {
                value += center * 5;

                if (piece->is_white) {
                    ++white_bishops;
                } else {
                    ++black_bishops;
                }
            } else if (piece->name == "Rook") {
                if (relative_row == 6) {
                    value += 20;
                }
            }

            white_score += piece->is_white ? value : -value;
        }
    }

    if (white_bishops >= 2) white_score += 25;
    if (black_bishops >= 2) white_score -= 25;

    return game.white_to_move() ? white_score : -white_score;
}

SearchResult choose_move(
    const Game& position,
    SearchLimits limits,
    const std::atomic_bool* cancel,
    Evaluator evaluator,
    const SearchResult* seed) {

    limits.max_depth =
        std::clamp(limits.max_depth, 1, maximum_depth);
    limits.time_ms =
        std::clamp(limits.time_ms, 1, 3600000);
    limits.max_nodes =
    std::clamp<std::uint64_t>(
        limits.max_nodes, 1, 1000000000ULL);
    limits.workers =
        std::clamp(limits.workers, 1, available_workers());

    Search search(
        limits,
        cancel,
        evaluator ? evaluator : evaluate);

    SearchResult result;

    // Legality probes temporarily modify Board, so use a private copy.
    Game root = position;

    if (root.finished()) {
        result.score_cp = terminal_score(root, 0);
    } else if (cancel &&
               cancel->load(std::memory_order_relaxed)) {
        result.interrupted = true;
    } else {
        // Establish a legal fallback even when the budget is tiny.
        // This initial pass deliberately ignores the deadline.
        auto moves = search.legal_moves(root, false);

        if (moves.empty()) {
            throw std::logic_error(
                "Unfinished root has no legal moves.");
        }

        order_moves(root, moves);

        result.move = moves.front();
        result.score_cp = std::clamp(
            search.evaluator(root),
            -evaluation_limit,
            evaluation_limit);

        // The caller must ensure the seed belongs to this exact position.
        // Checking legality alone cannot establish position identity.
        
        if (seed &&
            seed->move &&
            seed->completed_depth > 0 &&
            seed->completed_depth <= limits.max_depth) {

            const bool legal = std::any_of(
                moves.begin(), moves.end(),
                [&](const GameMove& move) {
                    return same_move(move, *seed->move);
                });

            if (legal) {
                result.move = seed->move;
                result.score_cp = seed->score_cp;
                result.completed_depth = seed->completed_depth;
            }
        }

        // If pondering already exceeded the normal depth setting,
        // attempt one more depth using this turn's time/node budget.
        const int target = limits.max_depth;

        if (!solved_result(result)) {
            for (int depth = result.completed_depth + 1;
                 depth <= target;
                 ++depth) {

                try {
                    const auto completed =
                        search.root(
                            root, moves, depth, result.move);

                    // Never replace a completed result with a partial pass.
                    result.move = completed.first;
                    result.score_cp = completed.second;
                    result.completed_depth = depth;

                    if (solved_result(result)) {
                        break;
                    }
                } catch (const Interrupted&) {
                    result.interrupted = true;
                    break;
                }
            }
        }
    }

    // These counters describe new work during this AI turn.
    // Previously completed pondering work is excluded.
    result.nodes =
        search.nodes.load(std::memory_order_relaxed);

    result.elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - search.started).count();

    return result;
}

std::vector<PonderReply> ponder(
    const Game& position,
    SearchLimits limits,
    const std::atomic_bool* cancel,
    Evaluator evaluator) {

    std::vector<PonderReply> replies;

    // A pondering session must always have a cancellation mechanism.
    if (!cancel ||
        cancel->load(std::memory_order_relaxed) ||
        position.finished()) {
        return replies;
    }

    limits.workers =
        std::clamp(limits.workers, 1, available_workers());

    // The duration of the human's turn supplies the pondering budget.
    // Zero disables these limits internally.
    limits.time_ms = 0;
    limits.max_nodes = 0;

    Search search(
        limits,
        cancel,
        evaluator ? evaluator : evaluate);

    Game root = position;

    try {
        const auto human_moves = search.legal_moves(root);
        replies.reserve(human_moves.size());

        for (const auto& move : human_moves) {
            replies.push_back(PonderReply{move, {}});
        }

        // Complete a shallow round across all possible human moves
        // before attempting the next depth.
        for (int depth = 1;
            depth <= std::min(
                limits.max_depth,
                std::clamp(limits.ponder_max_depth, 1, maximum_depth));
            ++depth) {
            bool searched_any = false;

            for (auto& entry : replies) {
                search.check_time();

                if (solved_result(entry.reply)) {
                    continue;
                }

                // Only the currently searched hypothetical position is
                // materialised. Other entries retain small result records.
                Game child = root;

                if (!child.play(entry.human_move)) {
                    throw std::logic_error(
                        "Engine rejected a pondering move.");
                }

                // A terminal human move needs no AI reply.
                if (child.finished()) {
                    continue;
                }

                auto moves = search.legal_moves(child);

                if (moves.empty()) {
                    throw std::logic_error(
                        "Unfinished pondering position has no legal moves.");
                }

                const auto before =
                    search.nodes.load(std::memory_order_relaxed);
                const auto started = Clock::now();

                const auto completed =
                    search.root(
                        child, moves, depth, entry.reply.move);

                // Publish only after this entire candidate pass completes.
                entry.reply.move = completed.first;
                entry.reply.score_cp = completed.second;
                entry.reply.completed_depth = depth;

                entry.reply.nodes +=
                    search.nodes.load(std::memory_order_relaxed) - before;

                entry.reply.elapsed_ms +=
                    std::chrono::duration_cast<
                        std::chrono::milliseconds>(
                            Clock::now() - started).count();

                searched_any = true;
            }

            if (!searched_any) {
                break;
            }
        }
    } catch (const Interrupted&) {
        // Stack unwinding releases partial search branches.
        // Completed reply records remain available to the GUI.
    }

    return replies;
}

} // namespace zerg