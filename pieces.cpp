
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
