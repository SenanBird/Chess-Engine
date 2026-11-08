#include <cassert>
#include <iostream>
#include <string>

#include "pieces.hpp"

int main() {
    Board board;
    const std::string move_limit =
        "75 moves per side without a pawn move or capture";

    // Repeat a quiet knight cycle from the starting position.
    const int cycle[4][4] = {
        {7, 6, 5, 5}, // White: g1-f3.
        {0, 6, 2, 5}, // Black: g8-f6.
        {5, 5, 7, 6}, // White: f3-g1.
        {2, 5, 0, 6}  // Black: f6-g8.
    };

    auto play_quiet_moves = [&](int count) {
        for (int i = 0; i < count; ++i) {
            const auto& move = cycle[i % 4];
            assert(board.try_move(move[0], move[1], move[2], move[3]));
        }
    };

    // The terminal would stop earlier for repetition. This test drives
    // Board directly to isolate the counter and check its exact boundary.
    board.set_start_pieces();
    play_quiet_moves(149);
    assert(board.automatic_draw_reason(false) != move_limit);

    // Black plays the 150th halfmove: g8-f6.
    assert(board.try_move(0, 6, 2, 5));
    assert(board.automatic_draw_reason(true) == move_limit);

    // At 149 halfmoves, a black pawn move should reset the counter.
    board.set_start_pieces();
    play_quiet_moves(149);
    assert(board.try_move(1, 4, 3, 4)); // e7-e5.
    assert(board.automatic_draw_reason(true).empty());

    // Set up a capture on f6 without changing the move counter.
    // Black's knight can capture the white bishop there on halfmove 150.
    board.set_start_pieces();
    play_quiet_moves(149);
    board.move_piece(7, 2, 2, 5); // Place the white bishop on f6.
    assert(board.try_move(0, 6, 2, 5)); // g8xf6.
    assert(board.automatic_draw_reason(true).empty());

    std::cout << "75-move threshold and reset tests passed.\n";
}