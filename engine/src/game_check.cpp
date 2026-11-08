#include "pieces.hpp"

#include <cstdlib>
#include <utility>

namespace {

// Piece generators propose destinations. The board checks king safety afterward.
std::vector<Move> candidate_moves(
    Piece& piece, int row, int col, const Board& board) {
    if (auto* pawn = dynamic_cast<Pawn*>(&piece))
        return pawn->all_pawn_moves_possible(row, col, board);
    if (auto* knight = dynamic_cast<Knight*>(&piece))
        return knight->all_knight_moves_possible(row, col, board);
    if (auto* bishop = dynamic_cast<Bishop*>(&piece))
        return bishop->all_bishop_moves_possible(row, col, board);
    if (auto* rook = dynamic_cast<Rook*>(&piece))
        return rook->all_rook_moves_possible(row, col, board);
    if (auto* queen = dynamic_cast<Queen*>(&piece))
        return queen->all_queen_moves_possible(row, col, board);
    if (auto* king = dynamic_cast<King*>(&piece))
        return king->all_king_moves_possible(row, col, board);

    return {};
}

// Create the chosen piece in the pawn's color. An unknown choice is invalid.
std::unique_ptr<Piece> make_promoted_piece(char choice, bool white) {
    switch (choice) {
        case 'q': return std::make_unique<Queen>(white);
        case 'r': return std::make_unique<Rook>(white);
        case 'b': return std::make_unique<Bishop>(white);
        case 'n': return std::make_unique<Knight>(white);
        default:  return nullptr;
    }
}

} // namespace

bool Board::in_check(bool white) const {
    // Find the king of the color we are checking.
    for (int king_row = 0; king_row < 8; ++king_row) {
        for (int king_col = 0; king_col < 8; ++king_col) {
            if (!is_king_here(king_row, king_col, !white))
                continue;

            // Use attack patterns directly. Pinned pieces still attack squares.
            for (int row = 0; row < 8; ++row) {
                for (int col = 0; col < 8; ++col) {
                    Piece* piece = get_piece(row, col, *this);
                    if (piece == nullptr || piece->is_white == white)
                        continue;

                    const int dr = std::abs(king_row - row);
                    const int dc = std::abs(king_col - col);

                    if (auto* pawn = dynamic_cast<Pawn*>(piece)) {
                        if (pawn->is_king_in_check(row, col, king_row, king_col, *this))
                            return true;
                    } else if (dynamic_cast<Knight*>(piece)) {
                        if ((dr == 2 && dc == 1) || (dr == 1 && dc == 2))
                            return true;
                    } else if (dynamic_cast<King*>(piece)) {
                        if (dr <= 1 && dc <= 1)
                            return true;
                    } else if (auto* rook = dynamic_cast<Rook*>(piece)) {
                        if (rook->pointed_at_opposing_king(row, col, *this))
                            return true;
                    } else if (auto* bishop = dynamic_cast<Bishop*>(piece)) {
                        if (bishop->pointed_at_opposing_king(row, col, *this))
                            return true;
                    } else if (auto* queen = dynamic_cast<Queen*>(piece)) {
                        if (queen->pointed_at_opposing_king(row, col, *this))
                            return true;
                    }
                }
            }
            return false;
        }
    }

    return false;
}

bool Board::is_en_passant(
    int from_row, int from_col, int to_row, int to_col) const {
    // The capture must land on the empty square crossed on the last move.
    if (out_of_bounds(to_row, to_col) ||
        to_row != en_passant_row || to_col != en_passant_col ||
        is_there(to_row, to_col)) {
        return false;
    }

    const auto* pawn =
        dynamic_cast<const Pawn*>(get_piece(from_row, from_col, *this));
    if (pawn == nullptr) {
        return false;
    }

    const int direction = pawn->is_white ? -1 : 1;
    if (to_row != from_row + direction ||
        std::abs(to_col - from_col) != 1) {
        return false;
    }

    // The captured pawn is beside the moving pawn, on its original rank.
    const auto* captured =
        dynamic_cast<const Pawn*>(get_piece(from_row, to_col, *this));
    return captured != nullptr && captured->is_white != pawn->is_white;
}

bool Board::leaves_king_in_check(
    int from_row, int from_col, int to_row, int to_col) {
    if (!is_there(from_row, from_col) ||
        out_of_bounds(to_row, to_col) ||
        (from_row == to_row && from_col == to_col)) {
        return true;
    }

    auto& source = grid[from_row][from_col];
    auto& target = grid[to_row][to_col];
    const bool white = source->is_white;
    const bool en_passant = is_en_passant(from_row, from_col, to_row, to_col);

    // Save any captured piece. Removing the adjacent pawn during an en
    // passant trial matters because doing so can open a line to our king.
    std::unique_ptr<Piece> captured;
    if (en_passant) {
        captured = std::move(grid[from_row][to_col]);
    } else {
        captured = std::move(target);
    }

    target = std::move(source);
    const bool king_in_check = in_check(white);

    // Restore the board, including the capture square, before returning.
    source = std::move(target);
    if (en_passant) {
        grid[from_row][to_col] = std::move(captured);
    } else {
        target = std::move(captured);
    }

    return king_in_check;
}

bool Board::is_legal_move(
    int from_row, int from_col, int to_row, int to_col) {
    if (out_of_bounds(from_row, from_col) || out_of_bounds(to_row, to_col)) {
        return false;
    }

    Piece* piece = get_piece(from_row, from_col, *this);
    if (piece == nullptr) {
        return false;
    }

    // Friendly pieces block the destination, and kings are never captured.
    const Piece* target = get_piece(to_row, to_col, *this);
    if (target != nullptr &&
        (target->is_white == piece->is_white || target->name == "King")) {
        return false;
    }

    // A two-file king move needs castling rights and a safe path.
    if (dynamic_cast<King*>(piece) != nullptr &&
        std::abs(to_col - from_col) == 2) {
        return can_castle(from_row, from_col, to_row, to_col);
    }

    // An en passant capture lands on an empty square.
    if (is_en_passant(from_row, from_col, to_row, to_col)) {
        return !leaves_king_in_check(from_row, from_col, to_row, to_col);
    }

    // For ordinary moves, check the piece's movement before trying the move.
    for (const Move& move : candidate_moves(*piece, from_row, from_col, *this)) {
        if (move.final_row == to_row && move.final_col == to_col) {
            return !leaves_king_in_check(from_row, from_col, to_row, to_col);
        }
    }

    return false;
}

bool Board::try_move(
    int from_row, int from_col, int to_row, int to_col, char promotion) {
    if (!is_legal_move(from_row, from_col, to_row, to_col)) {
        return false;
    }

    const Piece* piece = get_piece(from_row, from_col, *this);
    const bool white = piece->is_white;
    const bool pawn_move = dynamic_cast<const Pawn*>(piece) != nullptr;
    const bool promotes = pawn_move && to_row == (white ? 0 : 7);
    const bool en_passant = is_en_passant(from_row, from_col, to_row, to_col);
    const bool capture = en_passant || is_there(to_row, to_col);
    const bool double_step = pawn_move && std::abs(to_row - from_row) == 2;
    const bool castling = dynamic_cast<const King*>(piece) != nullptr &&
                          std::abs(to_col - from_col) == 2;

    // Validate the promotion choice before changing the position.
    std::unique_ptr<Piece> promoted_piece;
    if (promotes) {
        promoted_piece = make_promoted_piece(promotion, white);
        if (!promoted_piece) {
            return false;
        }
    } else if (promotion != '\0') {
        return false;
    }

    // Track lost castling rights before moving or capturing a piece.
    update_castling_rights(from_row, from_col, to_row, to_col);
    move_piece(from_row, from_col, to_row, to_col);

    if (castling) {
        const bool kingside = to_col == 6;
        move_piece(from_row, kingside ? 7 : 0, from_row, kingside ? 5 : 3);
    }
    if (en_passant) {
        grid[from_row][to_col].reset();
    }
    if (promoted_piece) {
        grid[to_row][to_col] = std::move(promoted_piece);
    }

    // Finish updating the position before recording it for the next player.
    en_passant_row = double_step ? (from_row + to_row) / 2 : -1;
    en_passant_col = double_step ? to_col : -1;
    record_position(!white, pawn_move, capture);

    return true;
}