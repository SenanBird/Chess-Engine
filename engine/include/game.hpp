#pragma once

#include <string>
#include <vector>

#include "pieces.hpp"

// A move uses the board's zero-based coordinates. Promotion is optional.
struct GameMove {
    int from_row;
    int from_col;
    int to_row;
    int to_col;
    char promotion = '\0';
};

// Game owns turn order and the result. Board owns the chess rules.
class Game {
public:
    Game();

    const Board& board() const { return board_; }
    bool white_to_move() const { return white_to_move_; }
    bool in_check() const { return in_check_; }
    bool finished() const { return !result_.empty(); }
    const std::string& result() const { return result_; }
    const std::string& reason() const { return reason_; }

    // The GUI can use this for highlights, and a bot can use it to pick a move.
    // Promotion destinations appear once per available promotion piece.
    std::vector<GameMove> legal_moves_from(int row, int col);

    // Rejected moves leave the board, turn, and move history unchanged.
    bool play(const GameMove& move);

    // Call once after a game finishes to append its record.
    bool save_finished_game(const std::string& path = "games.tsv") const;

private:
    Board board_;
    bool white_to_move_ = true;
    bool in_check_ = false;
    std::vector<std::string> moves_;
    std::string result_;
    std::string reason_;

    void update_status();
};