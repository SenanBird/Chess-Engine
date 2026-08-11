//main.cpp

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "pieces.hpp"




int main(){
    Board myboard;
    myboard.print_board();
    myboard.set_start_pieces();
    myboard.print_board();

    myboard.move_piece(6,1,5,1);

    myboard.print_board();

    return 0;
};