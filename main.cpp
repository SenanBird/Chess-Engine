//main.cpp

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "pieces.hpp"



int main() {
    Board myboard;
    myboard.set_start_pieces();

    std::cout << "=== Initial Board ===" << std::endl;
    myboard.print_board();


    Piece* piece = myboard.get_piece(6, 4, myboard);
    Pawn* pawn = dynamic_cast<Pawn*>(piece);

    std::cout << "Here are all the moves for the pawn at (6,4)" << std::endl;

    if (pawn != nullptr) {
        auto moves = pawn->all_pawn_moves_possible(6, 4, myboard);

        for (const Move& move : moves) {
            
            std::cout << move.final_row << ", "<< move.final_col << '\n';
        }
    }


    Piece* piece_rook = myboard.get_piece(7, 0, myboard);
    Rook* rook = dynamic_cast<Rook*>(piece_rook);
    auto rook_moves = rook->all_rook_moves_possible(0,1,myboard);
    std::cout << "Here are all the moves for a white rook at (0,1)" << std::endl;
    for(const Move& move : rook_moves){
        
        std::cout << move.final_row << ", "<< move.final_col << '\n';
    }

    Piece* piece_king = myboard.get_piece(7,4,myboard);
    King* king = dynamic_cast<King*>(piece_king);
    auto king_moves = king->all_king_moves_possible(0,3,myboard);
    std::cout << "Here are all the moves for a white king at (0,3)" << std::endl;
    for(const Move& move : king_moves){
        
        std::cout << move.final_row << ", "<< move.final_col << '\n';
    }
    

    

    

    

    return 0;
}