#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "pieces.hpp"


void Knight::knight_move(int start_row, int start_col, int final_row, int final_col, Board& board){


    if(is_pinned){
        std::cout << "The knight is pinned and can't move";
        return;
    }

    std::vector<Move> moves = Knight::all_knight_moves_possible(start_row, start_col, board);




    for(const Move& move: moves){

        
        if(move.final_row == final_row && move.final_col == final_col){
            
            board.move_piece(start_row, start_col, final_row, final_col);

            std::cout<<"Knight has moved from "<<(int)(start_row)<< " "<<(int)(start_col)<<"to "<<(int)(final_row)<< " "<<(int)(final_col);
            return;
        } 
            
        

    }


    std::cout << "This move is not possible\n";

    return;
}


std::vector<Move> Knight::all_knight_moves_possible(int start_row, int start_col,  const Board& board)const{

    std::vector<Move> all_moves;

    static constexpr int offsets[8][2]={{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};

    for(const auto& offset: offsets){

        const int target_row = start_row + offset[0];
        const int target_col = start_col + offset[1];

        if (board.out_of_bounds(target_row, target_col)) { //Out of bounds test
            continue;
        }
        if(!board.is_there(target_row,target_col)){ // There is nothing there
            all_moves.push_back({target_row,target_col,false});
        } else {  // There is something there, but it is not out of bounds
            if(board.can_take(target_row, target_col,is_white)){ // Checking if I can take
                all_moves.push_back({target_row,target_col,true}); // Adding the take to all_moves
            }  
        } 
    }     
    return all_moves;



    
}
    
