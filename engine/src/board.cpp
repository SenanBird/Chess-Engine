
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
};

void Board::set_start_pieces() {
    // Clear the old position so this works when restarting a game.
    for (auto& rank : grid) {
        for (auto& square : rank) {
            square.reset();
        }
    }

    // Neither side has moved yet, and there is no en passant target.
    en_passant_row = -1;
    en_passant_col = -1;

    for (int col = 0; col < 8; ++col) {
        grid[1][col] = std::make_unique<Pawn>(false);
        grid[6][col] = std::make_unique<Pawn>(true);
    }

    // Set up each back rank and give both sides their initial castling rights.
    for (bool white : {false, true}) {
        const int row = white ? 7 : 0;
        grid[row][0] = std::make_unique<Rook>(white);
        grid[row][1] = std::make_unique<Knight>(white);
        grid[row][2] = std::make_unique<Bishop>(white);
        grid[row][3] = std::make_unique<Queen>(white);
        grid[row][4] = std::make_unique<King>(white);
        grid[row][5] = std::make_unique<Bishop>(white);
        grid[row][6] = std::make_unique<Knight>(white);
        grid[row][7] = std::make_unique<Rook>(white);
        castling_rights[white][0] = true;
        castling_rights[white][1] = true;
    }

    // Record the finished starting position as its first occurrence.
    reset_draw_tracking();
}

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


Piece* Board::get_piece(int row, int col, const Board&) const {
    if (row<0 ||row>7||col<0||col>7){
        return nullptr;
    } else{
        return grid[row][col].get();
    }
};

bool Board::is_king_here(int row, int col, bool is_white) const {
    if (!is_there(row, col)) {
        return false;
    }

    const Piece& piece = *grid[row][col];
    return piece.name == "King" && piece.is_white != is_white;
};


