
#ifndef PIECES_H
#define PIECES_H
#include <vector>
#include <string>
#include <memory>


class Piece{
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
public:
    Board();
    void print_board();
    bool is_there(int row, int col) const;
    void set_start_pieces();
    void move_piece(int start_row, int start_col, int final_row, int final_col);
    bool can_take(int row, int col, bool is_white) const;
    bool out_of_bounds(int row, int col) const;
    Piece* get_piece(int row, int col, const Board& board) const;

};

class Pawn : public Piece {
public:
    Pawn(bool white) {
        name = "Pawn";
        is_white = white;
        symbol = white ? "wP" : "bP";
        is_pinned = false;
    }

    bool can_pawn_move_forward(int start_row, int start_col, const Board& board);

    void pawn_move(int start_row, int start_col, Board& board);

    std::vector<Move> all_pawn_moves_possible(int start_row, int start_col,  const Board& board);
};


class King : public Piece {
public:
    King(bool white) {
        name = "King";
        is_white = white;
        symbol = white ? "wK" : "bK";
    }


    void king_move(int start_row, int start_col, int final_row, int final_col, Board& board);

    std::vector<Move> all_king_moves_possible(int start_row, int start_col, const Board& board);
};


class Rook : public Piece {
public:
    Rook(bool white) {
        name = "Rook";
        is_white = white;
        symbol = white ? "wR" : "bR";
    }


    void Rook_move(int start_row, int start_col, int final_row, int final_col, Board& board);

    std::vector<Move> all_rook_moves_possible(int start_row, int start_col,  const Board& board);
};





#endif