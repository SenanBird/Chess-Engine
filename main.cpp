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

    Pawn white_pawn(true);

    // 1. Move White Pawn forward from (6, 4) to (5, 4)
    myboard.move_piece(6, 4, 5, 4);
    std::cout << "=== Move 1: White Pawn (6,4) -> (5,4) ===" << std::endl;
    myboard.print_board();

    // 2. Move Black Pawn forward from (1, 3) to (2, 3)
    myboard.move_piece(1, 3, 2, 3);
    std::cout << "=== Move 2: Black Pawn (1,3) -> (2,3) ===" << std::endl;
    myboard.print_board();

    // 3. Move White Pawn forward from (5, 4) to (4, 4)
    myboard.move_piece(5, 4, 4, 4);
    std::cout << "=== Move 3: White Pawn (5,4) -> (4,4) ===" << std::endl;
    myboard.print_board();

    // 4. Move Black Pawn forward from (2, 3) to (3, 3) (setting up the diagonal meeting)
    myboard.move_piece(2, 3, 3, 3);
    std::cout << "=== Move 4: Black Pawn (2,3) -> (3,3) ===" << std::endl;
    myboard.print_board();

    // 5. Check all possible moves for the White Pawn at (4, 4)
    std::cout << "=== Checking all_pawn_moves_possible() for White Pawn at (4,4) ===" << std::endl;
    std::vector<Move> moves = white_pawn.all_pawn_moves_possible(4, 4, myboard);
    for (const Move& m : moves) {
        std::cout << "  -> Valid move found: Row " << m.final_row << ", Col " << m.final_col << std::endl;
    }

    // 6. Execute the diagonal capture: White Pawn captures Black Pawn at (3, 3)
    myboard.move_piece(4, 4, 3, 3);
    std::cout << "=== Move 5 (Capture): White Pawn (4,4) -> (3,3) ===" << std::endl;
    myboard.print_board();

    return 0;
}