#include <cassert>
#include <iostream>

#include "pieces.hpp"

int main() {
    Board board;

    // Clear Black's queenside path and verify where both pieces land.
    board.set_start_pieces();
    board.move_piece(0, 1, 2, 1); // Knight off b8.
    board.move_piece(0, 2, 2, 2); // Bishop off c8.
    board.move_piece(0, 3, 2, 3); // Queen off d8.
    assert(board.try_move(0, 4, 0, 2));
    assert(dynamic_cast<King*>(board.get_piece(0, 2, board)) != nullptr);
    assert(dynamic_cast<Rook*>(board.get_piece(0, 3, board)) != nullptr);
    assert(!board.is_there(0, 4) && !board.is_there(0, 0));

    // Test e1, f1, and g1 separately. A black rook attacks down the
    // chosen file after the white pawn is moved out of its way.
    for (int attacked_col : {4, 5, 6}) {
        board.set_start_pieces();
        board.move_piece(7, 5, 5, 1); // Clear f1.
        board.move_piece(7, 6, 5, 2); // Clear g1.
        board.move_piece(6, attacked_col, 4, 0); // Clear the attack file.
        board.move_piece(0, 0, 4, attacked_col); // Aim at e1, f1, or g1.

        assert(board.in_check(true) == (attacked_col == 4));
        assert(!board.try_move(7, 4, 7, 6));

        // A rejected castle must leave both pieces on their original squares.
        assert(dynamic_cast<King*>(board.get_piece(7, 4, board)) != nullptr);
        assert(dynamic_cast<Rook*>(board.get_piece(7, 7, board)) != nullptr);
        assert(!board.is_there(7, 5) && !board.is_there(7, 6));
    }

    std::cout << "Castling tests passed.\n";
}