#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "pieces.hpp"


bool Pawn::can_pawn_move(int start_row, int start_col, const Board& board){

    int forward_direction = is_white ? -1 : 1;

    if(!board.is_there(start_row+forward_direction, start_col)){
        return true;
    }

    return false;
    
};

void Pawn::pawn_move(int start_row, int start_col, Board& board){
    int forward_direction = is_white ? -1 : 1;

    board.move_piece(start_row, start_col, start_row+forward_direction, start_col);

    std::cout<<"Pawn has moved from "<<(int)(start_row)<< " "<<(int)(start_col)<<"to "<<(int)(start_row+forward_direction)<< " "<<(int)(start_col);
};


std::vector<Move> Pawn::all_pawn_moves_possible(int start_row, int start_col, Board& board){

    std::vector<Move> all_moves;

    int forward_direction = is_white ? -1 : 1;

    if(Pawn::can_pawn_move( start_row, start_col, board)){
        all_moves.push_back({start_row+forward_direction, start_col});
    }

    // for the right side
    if(board.is_there(start_row+forward_direction, start_col+1)){
        if(board.can_take(start_row+forward_direction, start_col+1, is_white)){
            all_moves.push_back({start_row+forward_direction, start_col+1});
        }
    }

    // for the left side

    if(board.is_there(start_row+forward_direction, start_col-1)){
        if(board.can_take(start_row+forward_direction, start_col-1, is_white)){
            all_moves.push_back({start_row+forward_direction, start_col-1});
        }
    }

    return all_moves;

};
