#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "pieces.hpp"


void King::king_move(int start_row, int start_col, int final_row, int final_col, Board& board){

    std::vector<Move> moves = King::all_king_moves_possible(start_row, start_col, board);




    for(const Move& move: moves){

        
        if(move.final_row == final_row && move.final_col == final_col){
            
            board.move_piece(start_row, start_col, final_row, final_col);

            std::cout<<"King has moved from "<<(int)(start_row)<< " "<<(int)(start_col)<<"to "<<(int)(final_row)<< " "<<(int)(final_col);
            return;
        } 
            
        

    }


    std::cout << "This move is not possible";
    


   
};


std::vector<Move> King::all_king_moves_possible(int start_row, int start_col, const Board& board){
     std::vector<Move> all_moves;

     int forward_direction = is_white ? -1 : 1;


     for(int i=-1;i<2;i++){
        for(int j=-1;j<2;j++){
            if(j==0 && i==0){
                continue;
            }
            if(board.is_there(start_row + i, start_col + j)){
                if(board.can_take(start_row + i, start_col + j, is_white)){
                    all_moves.push_back({start_row + i, start_col + j, true});
                } 
            } else {
                if(!board.out_of_bounds(start_row + i, start_col + j)){
                    all_moves.push_back({start_row + i, start_col + j,  false});
                }   
            }      
     }
    }
    return all_moves;
};