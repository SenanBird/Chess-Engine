#include "pieces.hpp"

#include <utility>

bool Board::can_castle(int from_row, int from_col, int to_row, int to_col) {
    const auto* king =
        dynamic_cast<const King*>(get_piece(from_row, from_col, *this));
    if (king == nullptr) {
        return false;
    }

    // The king must start on e1 or e8 and move to c or g on the same rank.
    const bool white = king->is_white;
    const int home_row = white ? 7 : 0;
    if (from_row != home_row || to_row != home_row || from_col != 4 ||
        (to_col != 2 && to_col != 6)) {
        return false;
    }

    const bool kingside = to_col == 6;
    const int rook_col = kingside ? 7 : 0;
    const int step = kingside ? 1 : -1;
    const int crossing_col = from_col + step;

    const auto* rook =
        dynamic_cast<const Rook*>(get_piece(home_row, rook_col, *this));
    if (!castling_rights[white][kingside] ||
        rook == nullptr || rook->is_white != white) {
        return false;
    }

    // Every square between the king and rook must be empty.
    for (int col = crossing_col; col != rook_col; col += step) {
        if (is_there(home_row, col)) {
            return false;
        }
    }

    // The king cannot castle out of check or cross an attacked square.
    if (in_check(white) ||
        leaves_king_in_check(home_row, from_col, home_row, crossing_col)) {
        return false;
    }

    // Test the destination with both pieces in their final positions.
    auto& king_start = grid[home_row][from_col];
    auto& king_end = grid[home_row][to_col];
    auto& rook_start = grid[home_row][rook_col];
    auto& rook_end = grid[home_row][crossing_col];

    king_end = std::move(king_start);
    rook_end = std::move(rook_start);
    const bool destination_in_check = in_check(white);

    // Restore the original position. Only try_move() commits castling.
    rook_start = std::move(rook_end);
    king_start = std::move(king_end);
    return !destination_in_check;
}

void Board::update_castling_rights(
    int from_row, int from_col, int to_row, int to_col) {
    const Piece* moving = get_piece(from_row, from_col, *this);
    const Piece* captured = get_piece(to_row, to_col, *this);

    // A king move permanently removes both rights for that side.
    if (dynamic_cast<const King*>(moving) != nullptr) {
        castling_rights[moving->is_white][0] = false;
        castling_rights[moving->is_white][1] = false;
    }

    // A rook moving from its corner or being captured there removes that right.
    auto revoke_corner_right = [this](const Piece* piece, int row, int col) {
        const auto* rook = dynamic_cast<const Rook*>(piece);
        if (rook == nullptr || row != (rook->is_white ? 7 : 0)) {
            return;
        }

        if (col == 0 || col == 7) {
            castling_rights[rook->is_white][col == 7] = false;
        }
    };

    revoke_corner_right(moving, from_row, from_col);
    revoke_corner_right(captured, to_row, to_col);
}