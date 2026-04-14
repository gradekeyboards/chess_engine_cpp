#include "engine.hpp"
#include <chrono>
#include <iostream>

constexpr bool DEBUG = false;
int main()
{
    engine::Board board;
    engine::Eval eval(board);
    engine::Search search(board, eval);



    int depth{ 8 };
    int colour{ engine::Colour::white };
    static constexpr int player_colour{ engine::Colour::black };

    while (true)
    {
        board.print_board();
        
        if (colour == player_colour)
        {
            engine::Move player_move{};

            std::cout << "Enter the start square: " << '\n';
            std::cin >> player_move.start_square;

            std::cout << "Enter the destination square: " << '\n';
            std::cin >> player_move.destination_square;

            std::cout << "Enter the promotion piece: " << '\n';
            std::cin >> player_move.promotion_piece;

            bool is_legal_move{ false };
            engine::MovesInfo pseudo_legal_moves{ board.pseudo_legal_move_gen(colour) };
            for (int i{}; i < pseudo_legal_moves.count; i++)
            {
                engine::Move& move{ pseudo_legal_moves.moves[i] };
                if
                (
                    player_move.destination_square == move.destination_square
                    && player_move.start_square == move.start_square
                    && player_move.promotion_piece == move.promotion_piece
                )
                {
                    player_move = move;
                    is_legal_move = true;
                    break;
                }
            }

            if (is_legal_move)
            {
                board.make_move(player_move);
            }
            else
            {
                while (!is_legal_move)
                {
                    std::cout << "Sorry, that is not a legal move. Please try again: " << '\n';
                    std::cout << "Enter the start square: " << '\n';
                    std::cin >> player_move.start_square;

                    std::cout << "Enter the destination square: " << '\n';
                    std::cin >> player_move.destination_square;

                    std::cout << "Enter the promotion piece: " << '\n';
                    std::cin >> player_move.promotion_piece;

                    for (int i{}; i < pseudo_legal_moves.count; i++)
                    {
                        engine::Move& move{ pseudo_legal_moves.moves[i] };
                        if
                        (
                            player_move.destination_square == move.destination_square
                            && player_move.start_square == move.start_square
                            && player_move.promotion_piece == move.promotion_piece
                        )
                        {
                            player_move = move;
                            is_legal_move = true;
                            break;
                        }
                    }
                }
                board.make_move(player_move);
            }
        }
        else
        {
            engine::BestMove best_move{ search.get_best_move(depth, colour) };
            board.make_move(best_move.move);
            std::cout << "Eval for engine: " << best_move.eval << '\n';
        }

        colour = -colour;
    }

    if(DEBUG){
        // to test move gen(temporary)
        // auto moves = board.pseudo_legal_move_gen(engine::Colour::white);
        // std::cout << "Total moves: " << moves.count << '\n';

        int perft_colour{ engine::Colour::white };
        int perft_depth{ 6 };

        auto start{ std::chrono::steady_clock::now() };
        unsigned long long nodes{ board.perft(perft_depth, perft_colour) };
        auto end{ std::chrono::steady_clock::now() };

        std::chrono::duration<double> duration{ end - start };

        std::cout << nodes << " nodes in " << duration.count() << " seconds." << '\n';
        std::cout << "NPS: " << static_cast<unsigned long long>(nodes / duration.count()) << '\n';
    }

    return 0;
}

// g++ -O3 test.cpp -o test

// Up to depth 8 from the starting position is it right
// 84998978956 nodes in 2604.53 seconds.
// NPS: 32635008