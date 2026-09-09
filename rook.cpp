#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "pieces.hpp"



void Rook::Rook_move(int start_row, int start_col, int final_row, int final_col, Board& board){

    if(is_pinned){
        std::cout << "The rook is pinned and can't move";
        return;
    }

    std::vector<Move> moves = Rook::all_rook_moves_possible(start_row, start_col, board);




    for(const Move& move: moves){

        
        if(move.final_row == final_row && move.final_col == final_col){
            
            board.move_piece(start_row, start_col, final_row, final_col);

            std::cout<<"Rook has moved from "<<(int)(start_row)<< " "<<(int)(start_col)<<"to "<<(int)(final_row)<< " "<<(int)(final_col);
            return;
        } 
            
        

    }


    std::cout << "This move is not possible\n";
    


   
};


std::vector<Move> Rook::all_rook_moves_possible(int start_row, int start_col, const Board& board){
    std::vector<Move> all_moves;

    if(is_pinned){
        return all_moves;
    }

    bool up = true;
    bool down = true;
    bool left = true;
    bool right = true;



    //down
    int i = 1;

    while(down){
        if(!board.is_there(start_row + i,start_col) && !board.out_of_bounds(start_row + i,start_col)){ // There is nothing there, it is not out of bounds
            all_moves.push_back({start_row + i,start_col,false});
            i++;
        } else if(board.is_there(start_row + i,start_col) && !board.out_of_bounds(start_row + i,start_col)){  // There is something there, but it is not out of bounds
            if(board.can_take(start_row + i, start_col,is_white)){
                all_moves.push_back({start_row + i,start_col,true});
                down = false;
            } else { 
                down = false;
            }
        } else{
            down = false; // It must be out of bounds by this point
        }
        
    }

    //up
    i = 1;
    while(up){
        if(!board.is_there(start_row - i,start_col) && !board.out_of_bounds(start_row - i,start_col)){ // There is nothing there, it is not out of bounds
            all_moves.push_back({start_row - i,start_col,false});
            i++;
        } else if(board.is_there(start_row - i,start_col) && !board.out_of_bounds(start_row - i,start_col)){  // There is something there, but it is not out of bounds
            if(board.can_take(start_row - i, start_col,is_white)){
                all_moves.push_back({start_row - i,start_col,true});
                up = false;
            } else { 
                up = false;
            }
        } else{
            up = false; // It must be out of bounds by this point
        }
        
    }

    //right
    i = 1;
    while(right){
        if(!board.is_there(start_row, start_col + i) && !board.out_of_bounds(start_row, start_col + i)){ // There is nothing there, it is not out of bounds
            all_moves.push_back({start_row, start_col + i,false});
            i++;
        } else if(board.is_there(start_row, start_col + i) && !board.out_of_bounds(start_row, start_col + i)){  // There is something there, but it is not out of bounds
            if(board.can_take(start_row, start_col + i, is_white)){
                all_moves.push_back({start_row, start_col + i,true});
                right = false;
            } else { 
                right = false;
            }
        } else{
            right = false; // It must be out of bounds by this point
        }
        
    }

    //left
    i = 1;
    while(left){
        if(!board.is_there(start_row, start_col - i) && !board.out_of_bounds(start_row, start_col - i)){ // There is nothing there, it is not out of bounds
            all_moves.push_back({start_row,start_col - i,false});
            i++;
        } else if(board.is_there(start_row, start_col - i) && !board.out_of_bounds(start_row, start_col - i)){  // There is something there, but it is not out of bounds
            if(board.can_take(start_row, start_col - i, is_white)){
                all_moves.push_back({start_row, start_col - i,true});
                left = false;
            } else { 
                left = false;
            }
        } else{
            left = false; // It must be out of bounds by this point
        }
        
    }

    return all_moves;
    
}