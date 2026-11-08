#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "pieces.hpp"


bool Pawn::can_pawn_move_forward(int start_row, int start_col, const Board& board) const{

    if(is_pinned){
        std::cout << "Can't move as pawn is pinned.\n";
        return false;
    }

    int forward_direction = is_white ? -1 : 1;

    if(!board.is_there(start_row+forward_direction, start_col)){
        return true;
    }

    return false;
    
};

void Pawn::pawn_move(int start_row, int start_col, Board& board) {

    if(is_pinned){
        std::cout << "Can't move as pawn is pinned.\n";
        return;
    }

    int forward_direction = is_white ? -1 : 1;

    board.move_piece(start_row, start_col, start_row+forward_direction, start_col);

    std::cout<<"Pawn has moved from "<<(int)(start_row)<< " "<<(int)(start_col)<<"to "<<(int)(start_row+forward_direction)<< " "<<(int)(start_col);
};


std::vector<Move> Pawn::all_pawn_moves_possible(
    int start_row, int start_col, const Board& board) const {
    std::vector<Move> all_moves;

    if (is_pinned) {
        return all_moves;
    }

    const int direction = is_white ? -1 : 1;
    const int next_row = start_row + direction;
    const int starting_row = is_white ? 6 : 1;

    if (board.out_of_bounds(next_row, start_col)) {
        return all_moves;
    }

    if (!board.is_there(next_row, start_col)) {
        all_moves.push_back({next_row, start_col, false});

        const int double_row = next_row + direction;
        if (start_row == starting_row && !board.is_there(double_row, start_col)) {
            all_moves.push_back({double_row, start_col, false});
        }
    }

    for (int offset : {-1, 1}) {
        const int target_col = start_col + offset;
        if (board.can_take(next_row, target_col, is_white)) {
            all_moves.push_back({next_row, target_col, true});
        }
    }

    return all_moves;
}

bool Pawn::is_king_in_check(int pawn_row, int pawn_col, int king_row, int king_col, const Board& board) const {
    const int direction = is_white ? -1 : 1;

    return king_row == pawn_row + direction
        && std::abs(king_col - pawn_col) == 1
        && board.is_king_here(king_row, king_col, is_white);
}
