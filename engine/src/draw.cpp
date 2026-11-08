#include "pieces.hpp"

#include <algorithm>

std::string Board::repetition_key(bool white_to_move) {
    std::string key;
    key += white_to_move ? 'w' : 'b';

    // Include piece placement and castling rights. The same arrangement
    // can allow different moves if a king or rook moved and came back.
    for (const auto& rank : grid) {
        for (const auto& square : rank) {
            key += square ? square->symbol : ".";
            key += '/';
        }
    }

    for (const auto& side : castling_rights) {
        for (bool right : side) {
            key += right ? '1' : '0';
        }
    }

    // The en passant target only matters if the side to move can legally
    // capture there. A pinned pawn may be unable to use the target.
    if (en_passant_row >= 0) {
        const int source_row = en_passant_row + (white_to_move ? 1 : -1);
        for (int offset : {-1, 1}) {
            const int source_col = en_passant_col + offset;
            const Piece* piece = get_piece(source_row, source_col, *this);
            if (piece != nullptr && piece->is_white == white_to_move &&
                is_en_passant(source_row, source_col, en_passant_row, en_passant_col) &&
                is_legal_move(source_row, source_col, en_passant_row, en_passant_col)) {
                key += ':';
                key += static_cast<char>('a' + en_passant_col);
                key += static_cast<char>('8' - en_passant_row);
                break;
            }
        }
    }

    return key;
}

void Board::reset_draw_tracking() {
    halfmoves_since_progress = 0;
    position_history.clear();
    position_history.push_back(repetition_key(true));
}

void Board::record_position(bool white_to_move, bool pawn_move, bool capture) {
    // Each accepted move is one halfmove. Pawns and captures reset the count.
    if (pawn_move || capture) {
        halfmoves_since_progress = 0;
    } else {
        ++halfmoves_since_progress;
    }

    position_history.push_back(repetition_key(white_to_move));
}

bool Board::has_insufficient_material() const {
    int bishops = 0;
    int knights = 0;
    int bishop_square_color = -1;

    // Bare kings, one minor piece total, or bishops confined to the same
    // square color cannot produce mate. Other cases need separate analysis.
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            const Piece* piece = grid[row][col].get();
            if (piece == nullptr || dynamic_cast<const King*>(piece)) {
                continue;
            }

            if (dynamic_cast<const Knight*>(piece)) {
                ++knights;
            } else if (dynamic_cast<const Bishop*>(piece)) {
                const int color = (row + col) % 2;
                if (bishop_square_color != -1 && bishop_square_color != color) {
                    return false;
                }
                bishop_square_color = color;
                ++bishops;
            } else {
                return false;
            }
        }
    }

    return bishops + knights <= 1 || knights == 0;
}

std::string Board::automatic_draw_reason(bool white_to_move) {
    if (has_insufficient_material()) {
        return "Insufficient material";
    }

    if (halfmoves_since_progress >= 150) {
        return "75 moves per side without a pawn move or capture";
    }

    const std::string key = repetition_key(white_to_move);
    if (std::count(position_history.begin(), position_history.end(), key) >= 5) {
        return "Fivefold repetition";
    }

    return {};
}