#include "engine.hpp"
constexpr bool DEBUG = false;
int main()
{
    engine::Board board;
    board.print_board();

    if(DEBUG){
        // to test move gen(temporary)
        auto moves = board.pseudo_legal_move_gen();
        std::cout << "Total moves: " << moves.count << '\n';
    }

    return 0;
}

// g++ -O3 test.cpp -o test