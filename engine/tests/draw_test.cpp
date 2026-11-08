#include <cassert>
#include <iostream>

#include "pieces.hpp"

int main() {
    Board board;

    // Keep both kings and a white knight. Use the raw board move to
    // remove the other pieces while setting up this test position.
    board.set_start_pieces();
    int king_row = 7;
    int king_col = 4;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            const bool keep_square =
                (row == 0 && col == 4) || // Black king on e8.
                (row == 7 && col == 1) || // White knight on b1.
                (row == king_row && col == king_col);
            if (keep_square || !board.is_there(row, col)) {
                continue;
            }

            board.move_piece(king_row, king_col, row, col);
            king_row = row;
            king_col = col;
        }
    }

    // King and knight against a bare king cannot produce checkmate.
    assert(board.automatic_draw_reason(true) == "Insufficient material");

    // Resetting the same board should restore the full starting position.
    board.set_start_pieces();
    int piece_count = 0;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (board.is_there(row, col)) {
                ++piece_count;
            }
        }
    }

    assert(piece_count == 32);
    assert(board.automatic_draw_reason(true).empty());

    std::cout << "Material and board reset tests passed.\n";
}