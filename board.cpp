
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

void Board::print_board() {
    std::cout << "\n------------------------------------------\n";

    for (int i = 0; i < 8; ++i) {
        std::cout << "|";

        for (int j = 0; j < 8; ++j) {
            if (grid[i][j] == nullptr) {
                std::cout << "    |";
            } else {
                std::cout << " " << grid[i][j]->symbol << " |";
            }
        }

        std::cout << "\n------------------------------------------\n";
    }
}

void Board::set_start_pieces(){
    
    // BLACK PAWNS (Row 1) 
    for(int i = 0; i < 8; i++){
        grid[1][i]=std::make_unique<Pawn>(false);
    }

    // WHITE PAWNS (Row 6) 
    for(int i = 0; i < 8; i++){
        grid[6][i]=std::make_unique<Pawn>(true);
    }

    // White King (row 7, col 4)
    grid[7][4]=std::make_unique<King>(true);

    // Black King (row 0, col 3)
    grid[0][4]=std::make_unique<King>(false);

    //White Rook (row 7, col 0 and col 7)
    grid[7][0]=std::make_unique<Rook>(true);
    grid[7][7]=std::make_unique<Rook>(true);

    //Black Rook (row 0, col 0 and col 7)
    grid[0][0]=std::make_unique<Rook>(false);
    grid[0][7]=std::make_unique<Rook>(false);



};

void Board::move_piece(int start_row, int start_col, int final_row, int final_col){
    grid[final_row][final_col] = std::move(grid[start_row][start_col]);
};



bool Board::is_there(int row, int col) const {

    if (row<0 ||row>7||col<0||col>7){
        return false;
    }

    return grid[row][col] != nullptr;
};

bool Board::can_take(int row, int col, bool is_white) const {

    if (row<0 ||row>7||col<0||col>7){
        return false;
    }

    if (!is_there(row, col)) {
        return false;
    }

    return grid[row][col]->is_white != is_white;

};

bool Board:: out_of_bounds(int row, int col) const {
    if (row<0 ||row>7||col<0||col>7){
        return true;
    }

    return false;
};


Piece* Board::get_piece(int row, int col, const Board& board) const{
    if (row<0 ||row>7||col<0||col>7){
        return nullptr;
    } else{
        return grid[row][col].get();
    }
}


