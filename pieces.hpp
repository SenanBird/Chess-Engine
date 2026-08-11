
#ifndef PIECES_H
#define PIECES_H

#include <vector>
#include <string>
#include <memory>


class Piece{
public: 
    std::string name;
    char symbol;         
    bool is_white;       
    bool is_pinned = false; 

    virtual ~Piece() = default;


};

class Board {
private:
    std::vector<std::vector<std::unique_ptr<Piece>>> grid;
public:
    Board();
    void print_board();
    bool is_there(int row, int column) const;
    void set_start_pieces();
    void move_piece(int start_row, int start_col, int final_row, int final_col);

};

class Pawn : public Piece {
public:
    Pawn(bool white) {
        name = "Pawn";
        is_white = white;
        symbol = white ? 'P' : 'p';
    }

    bool can_pawn_move(int start_row, int start_col, const Board& board);

    void pawn_move(int start_row, int start_col, Board& board);
};





#endif