#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "game.hpp"

namespace {

struct PlayedGame {
    std::vector<GameMove> moves;
    std::string result;
    std::string reason;
};

// Use a separate deterministic seed for each game.
std::uint64_t mix_seed(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) *
        0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) *
        0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

std::vector<GameMove> all_legal_moves(Game& game) {
    std::vector<GameMove> moves;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            auto from = game.legal_moves_from(row, col);
            moves.insert(
                moves.end(),
                from.begin(),
                from.end());
        }
    }

    return moves;
}

std::string placement(const Board& board) {
    std::string result;

    for (int row = 0; row < 8; ++row) {
        if (row != 0) {
            result += '/';
        }

        int empty = 0;

        for (int col = 0; col < 8; ++col) {
            const Piece* piece =
                board.get_piece(row, col, board);

            if (!piece) {
                ++empty;
                continue;
            }

            if (empty != 0) {
                result += static_cast<char>('0' + empty);
                empty = 0;
            }

            char letter = piece->name == "Knight"
                ? 'N'
                : piece->name.front();

            if (!piece->is_white) {
                letter = static_cast<char>(
                    letter - 'A' + 'a');
            }

            result += letter;
        }

        if (empty != 0) {
            result += static_cast<char>('0' + empty);
        }
    }

    return result;
}

void run_one_game(
    std::uint64_t seed,
    unsigned max_plies,
    unsigned& completed_games,
    unsigned& capped_games) {
    Game game;
    std::mt19937_64 random(seed);
    std::vector<GameMove> played;

    for (unsigned ply = 0;
         ply < max_plies && !game.finished();
         ++ply) {
        auto legal = all_legal_moves(game);

        // An unfinished game must have a legal continuation.
        assert(!legal.empty());

        std::uniform_int_distribution<std::size_t> pick(
            0, legal.size() - 1);

        const GameMove move = legal[pick(random)];
        const bool side_before = game.white_to_move();

        assert(game.play(move));
        assert(game.white_to_move() != side_before);

        played.push_back(move);
    }

    if (game.finished()) {
        ++completed_games;
        assert(!game.result().empty());
        assert(!game.reason().empty());
    } else {
        ++capped_games;
        assert(played.size() == max_plies);
        assert(game.result().empty());

        // Hitting the test limit is not a chess draw.
        assert(!all_legal_moves(game).empty());
    }

    // Replay every chosen move through a fresh Game. This checks that
    // the recorded sequence is legal and reconstructs the same state.
    Game replay;

    for (const GameMove& move : played) {
        assert(replay.play(move));
    }

    assert(replay.white_to_move() ==
           game.white_to_move());
    assert(replay.finished() ==
           game.finished());
    assert(replay.result() ==
           game.result());
    assert(replay.reason() ==
           game.reason());
    assert(placement(replay.board()) ==
           placement(game.board()));
}

} // namespace

int main() {
    constexpr unsigned games = 20;
    constexpr unsigned max_plies = 300;
    constexpr std::uint64_t base_seed = 20261108;

    unsigned completed_games = 0;
    unsigned capped_games = 0;

    for (unsigned index = 0; index < games; ++index) {
        run_one_game(
            mix_seed(base_seed + index),
            max_plies,
            completed_games,
            capped_games);
    }

    assert(completed_games + capped_games == games);

    std::cout
        << "Random rules test passed: "
        << games << " games; "
        << completed_games << " completed; "
        << capped_games << " reached the test limit.\n";
}