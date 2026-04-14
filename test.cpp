#include "engine.hpp"
#include <chrono>
#include <iostream>

constexpr bool DEBUG = false;
int main()
{
    engine::Board board;

    int depth{ 6 };
    int colour{ engine::Colour::white };

    auto start{ std::chrono::steady_clock::now() };
    unsigned long long nodes{ board.perft(depth, colour) };
    auto end{ std::chrono::steady_clock::now() };

    std::chrono::duration<double> duration{ end - start };

    std::cout << nodes << " nodes in " << duration.count() << " seconds." << '\n';
    std::cout << "NPS: " << static_cast<unsigned long long>(nodes / duration.count()) << '\n';

    if(DEBUG){
        // to test move gen(temporary)
        auto moves = board.pseudo_legal_move_gen(engine::Colour::white);
        std::cout << "Total moves: " << moves.count << '\n';
    }

    return 0;
}

// g++ -O3 test.cpp -o test

// Up to depth 8 from the starting position is it right
// 84998978956 nodes in 2604.53 seconds.
// NPS: 32635008