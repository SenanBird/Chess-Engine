
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "pieces.hpp"




Board::Board(): grid(8){
    for(int i = 0; i<8 ; i++){
        grid[i].resize(8);
    };

};

void Board::print_board(){
    std::cout << "\n-----------\n";
    for(int i=0;i<8;i++){
        for(int j=0; j<8; j++){
            if( grid[i][j]==nullptr){
                std::cout << ". ";
            } else {
                std::cout << grid[i][j]->symbol<< " ";
            }
            
        }
        std::cout << "\n";
    }
    std::cout<<"-----------\n";
};

void Board::set_start_pieces(){
    
    // BLACK PAWNS (Row 1) 
    for(int i = 0; i < 8; i++){
        grid[1][i]=std::make_unique<Pawn>(false);
    }

    // WHITE PAWNS (Row 6) 
    for(int i = 0; i < 8; i++){
        grid[6][i]=std::make_unique<Pawn>(true);

    }

    
};

void Board::move_piece(int start_row, int start_col, int final_row, int final_col){
    grid[final_row][final_col] = std::move(grid[start_row][start_col]);
}



bool Board::is_there(int row, int column) const {

    if (row<0 ||row>7||column<0||column>7){
        return false;
    }

    return grid[row][column] != nullptr;
};

bool Board::can_take(int row, int col, bool is_white) const {

    if (!is_there(row, col)) {
        return false;
    }


    return grid[row][col]->is_white != is_white;


};