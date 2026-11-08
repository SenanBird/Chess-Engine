#include "game.hpp"

#include <fstream>

namespace {

// Save the same coordinate notation the terminal accepts, including promotion.
std::string move_text(const GameMove& move) {
    std::string text;
    text += static_cast<char>('a' + move.from_col);
    text += static_cast<char>('8' - move.from_row);
    text += static_cast<char>('a' + move.to_col);
    text += static_cast<char>('8' - move.to_row);

    if (move.promotion != '\0') {
        text += move.promotion;
    }
    return text;
}

// Encode the piece-placement field of FEN for the existing history format.
std::string final_placement(const Board& board) {
    std::string placement;

    for (int row = 0; row < 8; ++row) {
        if (row != 0) {
            placement += '/';
        }

        int empty = 0;
        for (int col = 0; col < 8; ++col) {
            const Piece* piece = board.get_piece(row, col, board);
            if (piece == nullptr) {
                ++empty;
                continue;
            }

            if (empty != 0) {
                placement += static_cast<char>('0' + empty);
                empty = 0;
            }

            char letter = piece->name == "Knight" ? 'N' : piece->name.front();
            if (!piece->is_white) {
                letter = static_cast<char>(letter - 'A' + 'a');
            }
            placement += letter;
        }

        if (empty != 0) {
            placement += static_cast<char>('0' + empty);
        }
    }

    return placement;
}

} // namespace

Game::Game() {
    board_.set_start_pieces();
    update_status();
}

std::vector<GameMove> Game::legal_moves_from(int row, int col) {
    std::vector<GameMove> moves;
    const Piece* piece = board_.get_piece(row, col, board_);
    if (finished() || piece == nullptr || piece->is_white != white_to_move_) {
        return moves;
    }

    const bool pawn = dynamic_cast<const Pawn*>(piece) != nullptr;
    const int promotion_row = white_to_move_ ? 0 : 7;

    // Board checks the complete move, including castling and en passant.
    for (int to_row = 0; to_row < 8; ++to_row) {
        for (int to_col = 0; to_col < 8; ++to_col) {
            if (!board_.is_legal_move(row, col, to_row, to_col)) {
                continue;
            }

            if (pawn && to_row == promotion_row) {
                for (char choice : {'q', 'r', 'b', 'n'}) {
                    moves.push_back({row, col, to_row, to_col, choice});
                }
            } else {
                moves.push_back({row, col, to_row, to_col});
            }
        }
    }

    return moves;
}

bool Game::play(const GameMove& move) {
    const Piece* piece = board_.get_piece(move.from_row, move.from_col, board_);
    if (finished() || piece == nullptr || piece->is_white != white_to_move_) {
        return false;
    }

    if (!board_.try_move(move.from_row, move.from_col,
                         move.to_row, move.to_col, move.promotion)) {
        return false;
    }

    // Advance the turn only once Board has committed the move.
    moves_.push_back(move_text(move));
    white_to_move_ = !white_to_move_;
    update_status();
    return true;
}

void Game::update_status() {
    in_check_ = board_.in_check(white_to_move_);

    // Stop searching as soon as one piece has a legal move.
    bool has_move = false;
    for (int row = 0; row < 8 && !has_move; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (!legal_moves_from(row, col).empty()) {
                has_move = true;
                break;
            }
        }
    }

    // Checkmate and stalemate take precedence over the draw counter.
    if (!has_move) {
        if (in_check_) {
            result_ = white_to_move_ ? "0-1" : "1-0";
            reason_ = "checkmate";
        } else {
            result_ = "1/2-1/2";
            reason_ = "stalemate";
        }
        return;
    }

    reason_ = board_.automatic_draw_reason(white_to_move_);
    if (!reason_.empty()) {
        result_ = "1/2-1/2";
    }
}

bool Game::save_finished_game(const std::string& path) const {
    if (!finished()) {
        return false;
    }

    // Preserve the existing TSV columns and append without overwriting history.
    std::ofstream file(path, std::ios::app | std::ios::ate);
    if (!file) {
        return false;
    }

    if (file.tellp() == std::streampos(0)) {
        file << "result\treason\tmoves\tfinal_placement\n";
    }

    file << result_ << '\t' << reason_ << '\t';
    for (std::size_t i = 0; i < moves_.size(); ++i) {
        if (i != 0) {
            file << ' ';
        }
        file << moves_[i];
    }
    file << '\t' << final_placement(board_) << '\n';

    file.close();
    return static_cast<bool>(file);
}