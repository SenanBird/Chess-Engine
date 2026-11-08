

#pragma once

#include <memory>
#include <string>
#include <vector>

class Piece {
public:
    std::string name;
    std::string symbol;
    bool is_white;
    bool is_pinned = false;
    virtual ~Piece() = default;
};

struct Move {
    int final_row;
    int final_col;
    bool can_take_piece;
};

class Board {
private:
    std::vector<std::vector<std::unique_ptr<Piece>>> grid;
    int en_passant_row = -1;
    int en_passant_col = -1;

    bool is_en_passant(int from_row, int from_col,
                       int to_row, int to_col) const;

    bool castling_rights[2][2] = {};

    bool can_castle(int from_row, int from_col, int to_row, int to_col);
    void update_castling_rights(int from_row, int from_col,
                                int to_row, int to_col);

    std::vector<std::string> position_history;
    int halfmoves_since_progress = 0;

    std::string repetition_key(bool white_to_move);
    bool has_insufficient_material() const;
    void reset_draw_tracking();
    void record_position(bool white_to_move, bool pawn_move, bool capture);

public:
    Board();
    Board(const Board& other);
    Board& operator=(const Board& other);
    Board(Board&&) noexcept = default;
    Board& operator=(Board&&) noexcept = default;

    void print_board();
    void set_start_pieces();
    bool out_of_bounds(int row, int col) const;
    bool is_there(int row, int col) const;
    bool can_take(int row, int col, bool is_white) const;
    Piece* get_piece(int row, int col, const Board& board) const;
    bool is_king_here(int row, int col, bool is_white) const;

    void move_piece(int start_row, int start_col, int final_row, int final_col);
    bool in_check(bool white) const;

    bool leaves_king_in_check(int from_row, int from_col,
                              int to_row, int to_col);

    bool is_legal_move(int from_row, int from_col, int to_row, int to_col);

    bool try_move(int from_row, int from_col, int to_row, int to_col,
                  char promotion = '\0');

    std::string automatic_draw_reason(bool white_to_move);
};

class Pawn : public Piece {
public:
    Pawn(bool white) {
        name = "Pawn";
        is_white = white;
        symbol = white ? "wP" : "bP";
    }

    bool can_pawn_move_forward(int row, int col, const Board& board) const;
    void pawn_move(int row, int col, Board& board);
    std::vector<Move> all_pawn_moves_possible(
        int row, int col, const Board& board) const;
    bool is_king_in_check(int pawn_row, int pawn_col,
                          int king_row, int king_col,
                          const Board& board) const;
};

class King : public Piece {
public:
    King(bool white) {
        name = "King";
        is_white = white;
        symbol = white ? "wK" : "bK";
    }

    void king_move(int row, int col, int final_row, int final_col, Board& board);
    std::vector<Move> all_king_moves_possible(
        int row, int col, const Board& board);
};

class Rook : public Piece {
public:
    Rook(bool white) {
        name = "Rook";
        is_white = white;
        symbol = white ? "wR" : "bR";
    }

    void Rook_move(int row, int col, int final_row, int final_col, Board& board);
    std::vector<Move> all_rook_moves_possible(
        int row, int col, const Board& board);
    bool pointed_at_opposing_king(
        int row, int col, const Board& board);
};

class Bishop : public Piece {
public:
    Bishop(bool white) {
        name = "Bishop";
        is_white = white;
        symbol = white ? "wB" : "bB";
    }

    void bishop_move(int row, int col, int final_row, int final_col, Board& board);
    std::vector<Move> all_bishop_moves_possible(
        int row, int col, const Board& board) const;
    bool pointed_at_opposing_king(
        int row, int col, const Board& board) const;
};

class Knight : public Piece {
public:
    Knight(bool white) {
        name = "Knight";
        is_white = white;
        symbol = white ? "wKn" : "bKn";
    }

    void knight_move(int row, int col, int final_row, int final_col, Board& board);
    std::vector<Move> all_knight_moves_possible(
        int row, int col, const Board& board) const;
};

class Queen : public Piece {
public:
    Queen(bool white) {
        name = "Queen";
        is_white = white;
        symbol = white ? "wQ" : "bQ";
    }

    void queen_move(int row, int col, int final_row, int final_col, Board& board);
    std::vector<Move> all_queen_moves_possible(
        int row, int col, const Board& board) const;
    bool pointed_at_opposing_king(
        int row, int col, const Board& board) const;
};