#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "pieces.hpp"



void Queen::queen_move(int start_row, int start_col, int final_row, int final_col, Board& board){


    if(is_pinned){
        std::cout << "The queen is pinned and can't move";
        return;
    }

    std::vector<Move> moves = Queen::all_queen_moves_possible(start_row, start_col, board);




    for(const Move& move: moves){

        
        if(move.final_row == final_row && move.final_col == final_col){
            
            board.move_piece(start_row, start_col, final_row, final_col);

            std::cout<<"Queen has moved from "<<(int)(start_row)<< " "<<(int)(start_col)<<"to "<<(int)(final_row)<< " "<<(int)(final_col);
            return;
        } 
            
        

    }


    std::cout << "This move is not possible\n";

    return;
};

std::vector<Move> Queen::all_queen_moves_possible(int start_row, int start_col,  const Board& board) const{
    std::vector<Move> all_moves;


    bool upper_right = true;
    bool upper_left = true;
    bool down_right = true;
    bool down_left = true;


    // diagonally down-right
    int i = 1;
    while(down_right){
        if(!board.is_there(start_row + i,start_col+i) && !board.out_of_bounds(start_row + i,start_col+i)){ // There is nothing there, it is not out of bounds
            all_moves.push_back({start_row + i,start_col+i,false});
            i++;
        } else if(board.is_there(start_row + i,start_col+i) && !board.out_of_bounds(start_row + i,start_col+i)){  // There is something there, but it is not out of bounds
            if(board.can_take(start_row + i, start_col+i,is_white)){
                all_moves.push_back({start_row + i,start_col+i,true});
                down_right = false;
            } else { 
                down_right = false;
            }
        } else{
            down_right = false; // It must be out of bounds by this point
        }
        
    }

    //diagonally down left
    i=1;
    while(down_left){
        if(!board.is_there(start_row + i,start_col-i) && !board.out_of_bounds(start_row + i,start_col-i)){ // There is nothing there, it is not out of bounds
            all_moves.push_back({start_row + i,start_col-i,false});
            i++;
        } else if(board.is_there(start_row + i,start_col-i) && !board.out_of_bounds(start_row + i,start_col-i)){  // There is something there, but it is not out of bounds
            if(board.can_take(start_row + i, start_col-i,is_white)){
                all_moves.push_back({start_row + i,start_col-i,true});
                down_left = false;
            } else { 
                down_left = false;
            }
        } else{
            down_left = false; // It must be out of bounds by this point
        }
        
    }

    //diagonally upper left
    i = -1;

    while(upper_left){
        if(!board.is_there(start_row + i,start_col+i) && !board.out_of_bounds(start_row + i,start_col+i)){ // There is nothing there, it is not out of bounds
            all_moves.push_back({start_row + i,start_col+i,false});
            i--;
        } else if(board.is_there(start_row + i,start_col+i) && !board.out_of_bounds(start_row + i,start_col+i)){  // There is something there, but it is not out of bounds
            if(board.can_take(start_row + i, start_col+i,is_white)){
                all_moves.push_back({start_row + i,start_col+i,true});
                upper_left = false;
            } else { 
                upper_left = false;
            }
        } else{
            upper_left = false; // It must be out of bounds by this point
        }
        
    }

    //diagonally upper right

    i=-1;
    while(upper_right){
        if(!board.is_there(start_row + i,start_col-i) && !board.out_of_bounds(start_row + i,start_col-i)){ // There is nothing there, it is not out of bounds
            all_moves.push_back({start_row + i,start_col-i,false});
            i--;
        } else if(board.is_there(start_row + i,start_col-i) && !board.out_of_bounds(start_row + i,start_col-i)){  // There is something there, but it is not out of bounds
            if(board.can_take(start_row + i, start_col-i,is_white)){
                all_moves.push_back({start_row + i,start_col-i,true});
                upper_right = false;
            } else { 
                upper_right = false;
            }
        } else{
            upper_right = false; // It must be out of bounds by this point
        }
        
    }

    bool up = true;
    bool down = true;
    bool left = true;
    bool right = true;



    //down
    i = 1;

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

};


bool Queen::pointed_at_opposing_king(
    int start_row, int start_col, const Board& board) const {
    for (int dr : {-1, 0, 1}) {
        for (int dc : {-1, 0, 1}) {
            if (dr == 0 && dc == 0) {
                continue;
            }

            int row = start_row + dr;
            int col = start_col + dc;

            while (!board.out_of_bounds(row, col)) {
                if (board.is_there(row, col)) {
                    if (board.is_king_here(row, col, is_white)) {
                        return true;
                    }
                    break;
                }

                row += dr;
                col += dc;
            }
        }
    }

    return false;
}