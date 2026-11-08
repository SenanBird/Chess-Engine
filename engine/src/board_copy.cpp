

#include "pieces.hpp"

#include <stdexcept>
#include <utility>

namespace {

std::unique_ptr<Piece> clone_piece(const Piece& piece) {
    if (const auto* p = dynamic_cast<const Pawn*>(&piece))
        return std::make_unique<Pawn>(*p);
    if (const auto* p = dynamic_cast<const Knight*>(&piece))
        return std::make_unique<Knight>(*p);
    if (const auto* p = dynamic_cast<const Bishop*>(&piece))
        return std::make_unique<Bishop>(*p);
    if (const auto* p = dynamic_cast<const Rook*>(&piece))
        return std::make_unique<Rook>(*p);
    if (const auto* p = dynamic_cast<const Queen*>(&piece))
        return std::make_unique<Queen>(*p);
    if (const auto* p = dynamic_cast<const King*>(&piece))
        return std::make_unique<King>(*p);

    throw std::logic_error("Cannot copy an unknown piece type.");
}

} // namespace

Board::Board(const Board& other)
    : grid(other.grid.size()),
      en_passant_row(other.en_passant_row),
      en_passant_col(other.en_passant_col),
      position_history(other.position_history),
      halfmoves_since_progress(other.halfmoves_since_progress) {
    for (std::size_t row = 0; row < other.grid.size(); ++row) {
        grid[row].resize(other.grid[row].size());

        for (std::size_t col = 0; col < other.grid[row].size(); ++col) {
            if (other.grid[row][col]) {
                grid[row][col] = clone_piece(*other.grid[row][col]);
            }
        }
    }

    for (int color = 0; color < 2; ++color) {
        for (int wing = 0; wing < 2; ++wing) {
            castling_rights[color][wing] = other.castling_rights[color][wing];
        }
    }
}

Board& Board::operator=(const Board& other) {
    if (this != &other) {
        Board copy(other);
        *this = std::move(copy);
    }
    return *this;
}