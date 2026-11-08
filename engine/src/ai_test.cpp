#include "ai.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

GameMove move(const std::string& text) {
    return {'8' - text[1], text[0] - 'a',
            '8' - text[3], text[2] - 'a',
            text.size() == 5 ? text[4] : '\0'};
}

bool equal(const GameMove& a, const GameMove& b) {
    return a.from_row == b.from_row &&
           a.from_col == b.from_col &&
           a.to_row == b.to_row &&
           a.to_col == b.to_col &&
           a.promotion == b.promotion;
}

void play(Game& game, const std::string& text) {
    if (!game.play(move(text))) {
        std::cerr << "AI test setup: rejected move "
                  << text << '\n';
        std::abort();
    }
}

std::vector<GameMove> legal(Game& game) {
    std::vector<GameMove> result;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            auto moves = game.legal_moves_from(row, col);
            result.insert(result.end(), moves.begin(), moves.end());
        }
    }

    return result;
}

void equivalent(Game a, Game b) {
    assert(a.white_to_move() == b.white_to_move());
    assert(a.in_check() == b.in_check());
    assert(a.result() == b.result());
    assert(a.reason() == b.reason());

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            const auto* pa =
                a.board().get_piece(row, col, a.board());
            const auto* pb =
                b.board().get_piece(row, col, b.board());

            assert(bool(pa) == bool(pb));

            if (pa) {
                assert(pa->name == pb->name);
                assert(pa->is_white == pb->is_white);
                assert(pa->is_pinned == pb->is_pinned);
                assert(pa != pb);
            }
        }
    }

    const auto ma = legal(a);
    const auto mb = legal(b);

    assert(ma.size() == mb.size());

    for (std::size_t i = 0; i < ma.size(); ++i) {
        assert(equal(ma[i], mb[i]));
    }
}

} // namespace

int main() {
    zerg::SearchLimits limits;
    limits.max_depth = 1;
    limits.time_ms = 60000;
    limits.max_nodes = 1000000;
    limits.workers = 1;

    // Mate in one must take priority over static evaluation.
    Game mate;
    play(mate, "f2f3");
    play(mate, "e7e5");
    play(mate, "g2g4");

    const Game before = mate;
    const auto winning = zerg::choose_move(mate, limits);

    assert(winning.completed_depth == 1);
    assert(winning.move);
    assert(equal(*winning.move, move("d8h4")));
    assert(winning.score_cp == 29999);
    equivalent(mate, before);

    Game finished = mate;
    play(finished, "d8h4");
    assert(finished.finished());
    assert(!zerg::choose_move(finished, limits).move);

    // Tiny budgets still return a legal fallback.
    Game initial;
    auto tiny = limits;
    tiny.max_nodes = 1;

    const auto fallback = zerg::choose_move(initial, tiny);
    assert(fallback.move);
    assert(fallback.completed_depth == 0);
    assert(fallback.interrupted);
    assert(fallback.nodes <= 1);

    Game fallback_check = initial;
    assert(fallback_check.play(*fallback.move));

    std::atomic_bool canceled{true};
    const auto canceled_result =
        zerg::choose_move(initial, limits, &canceled);

    assert(canceled_result.interrupted);
    assert(canceled_result.nodes == 0);
    assert(!canceled_result.move);

    // Deep copies retain en passant.
    Game ep;
    for (const auto& text :
         {"e2e4", "a7a6", "e4e5", "d7d5"}) {
        play(ep, text);
    }

    Game ep_child = ep;
    play(ep_child, "e5d6");

    assert(ep.board().is_there(3, 3));
    assert(ep.board().is_there(3, 4));
    assert(!ep_child.board().is_there(3, 3));

    // Deep copies retain castling rights.
    Game castle;
    for (const auto& text :
         {"e2e4", "e7e5", "g1f3", "b8c6",
          "f1c4", "g8f6"}) {
        play(castle, text);
    }

    Game castle_child = castle;
    play(castle_child, "e1g1");

    assert(castle.board().is_there(7, 4));
    assert(castle.board().is_there(7, 7));
    assert(castle_child.board().get_piece(
        7, 6, castle_child.board())->name == "King");
    assert(castle_child.board().get_piece(
        7, 5, castle_child.board())->name == "Rook");

    // Deep copies retain repetition history.
    Game repetition;

    for (int cycle = 0; cycle < 3; ++cycle) {
        for (const auto& text :
             {"g1f3", "g8f6", "f3g1", "f6g8"}) {
            play(repetition, text);
        }
    }

    assert(!repetition.finished());

    Game repetition_child = repetition;
    for (const auto& text :
         {"g1f3", "g8f6", "f3g1", "f6g8"}) {
        play(repetition_child, text);
    }

    assert(repetition_child.finished());
    assert(repetition_child.result() == "1/2-1/2");
    assert(!repetition.finished());

    // Alpha-beta and full-width negamax agree at fixed depth.
    limits.max_depth = 2;
    limits.alpha_beta = false;
    const auto plain = zerg::choose_move(initial, limits);

    limits.alpha_beta = true;
    const auto serial = zerg::choose_move(initial, limits);

    assert(plain.completed_depth == 2);
    assert(serial.completed_depth == 2);
    assert(plain.score_cp == serial.score_cp);
    assert(plain.move && serial.move);
    assert(equal(*plain.move, *serial.move));
    assert(serial.nodes <= plain.nodes);

    // Root parallelism shares a single node budget.
    limits.workers =
        std::min(4, zerg::available_workers());
    const auto parallel =
        zerg::choose_move(initial, limits);

    assert(parallel.completed_depth == 2);
    assert(parallel.score_cp == serial.score_cp);
    assert(parallel.move);
    assert(equal(*parallel.move, *serial.move));
    assert(parallel.nodes <= limits.max_nodes);

    // A completed result survives an insufficient new budget.
    auto resume_limits = limits;
    resume_limits.max_nodes = 1;

    const auto resumed = zerg::choose_move(
        initial, resume_limits,
        nullptr, zerg::evaluate, &serial);

    assert(resumed.completed_depth >= serial.completed_depth);
    assert(resumed.move);
    assert(equal(*resumed.move, *serial.move));
    assert(resumed.nodes <= 1);

    // Already cancelled pondering does not start any search.
    std::atomic_bool stop{true};
    assert(zerg::ponder(initial, limits, &stop).empty());

    std::cout
        << "AI tests passed.\n"
        << "Serial alpha-beta: " << serial.nodes
        << " nodes, " << serial.elapsed_ms << " ms\n"
        << "Parallel alpha-beta: " << parallel.nodes
        << " nodes, " << parallel.elapsed_ms << " ms\n";
}