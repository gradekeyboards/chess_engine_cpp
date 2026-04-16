#include "engine.hpp"
#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <sstream>

static constexpr bool DEBUG = false;
static constexpr bool PLAY = false;
static constexpr bool UCI = true;

int main()
{
    engine::Board board;
    engine::Eval eval(board);
    engine::Search search(board, eval);



    if (PLAY)
    {
        int depth{ 7 };
        int colour{ engine::Colour::white };
        static constexpr int player_colour{ engine::Colour::white };

        std::unordered_map<int, std::string> index_to_string{ engine::Board::get_index_to_string() };
        std::unordered_map<std::string, int> string_to_index{ engine::Board::get_string_to_index() };

        while (true)
        {
            board.print_board();
            
            if (colour == player_colour)
            {
                engine::Move player_move{};

                std::cout << "Enter the start square: " << '\n';
                std::string start_square{};
                std::cin >> start_square;
                player_move.start_square = string_to_index[start_square];

                std::cout << "Enter the destination square: " << '\n';
                std::string destination_square{};
                std::cin >> destination_square;
                player_move.destination_square = string_to_index[destination_square];

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
                        std::cout << "Enter the start square: " << '\n';
                        std::string start_square{};
                        std::cin >> start_square;
                        player_move.start_square = string_to_index[start_square];

                        std::cout << "Enter the destination square: " << '\n';
                        std::string destination_square{};
                        std::cin >> destination_square;
                        player_move.destination_square = string_to_index[destination_square];

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
                std::cout << "Eval for engine: " << static_cast<double>(best_move.eval) / 100 << '\n';
            }

            colour = -colour;
        }
    }
    else if (DEBUG){
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
    else if (UCI)
    {
        int colour{ engine::Colour::white };

        std::unordered_map<int, std::string> index_to_string{ engine::Board::get_index_to_string() };
        std::unordered_map<std::string, int> string_to_index{ engine::Board::get_string_to_index() };
        std::unordered_map<char, int> string_to_piece{};

        std::array<char, 7> piece_to_string{ 'x', 'p', 'n', 'b', 'r', 'q', 'k'  };
        for (int i{}; i < piece_to_string.size(); i++)
        {
            string_to_piece[piece_to_string[i]] = i;
        }

        std::string input{};
        while (std::getline(std::cin, input))
        {
            std::istringstream input_stream(input);
            std::string word{};

            while (input_stream >> word)
            {
                if (word == "uci")
                {
                    std::cout << "id name Engine" << std::endl;
                    std::cout << "id author Us" << std::endl;
                    std::cout << "uciok" << std::endl;
                }
                else if (word == "isready")
                {
                    std::cout << "readyok" << '\n';
                    std::cout << std::flush;
                }
                else if (word == "position")
                {
                    std::string fen_or_moves{};
                    input_stream >> fen_or_moves;

                    if (fen_or_moves == "fen")
                    {
                        // TODO: make a function that converts from fen to board + correct board state
                        continue;
                    }
                    else if (fen_or_moves == "startpos")
                    {
                        board = engine::Board();
                        colour = engine::Colour::white;

                        std::string next_move{};
                        while (input_stream >> next_move)
                        {
                            if (next_move == "moves")
                            {
                                continue;
                            }

                            std::string start_square{};
                            std::string destination_square{};
                            char promotion_piece{};

                            for (int i{}; i < next_move.size(); i++)
                            {
                                if (i <= 1)
                                {
                                    start_square += next_move[i];
                                }
                                else if (i <= 3)
                                {
                                    destination_square += next_move[i];
                                }
                                else
                                {
                                    promotion_piece += next_move[i];
                                }
                            }

                            engine::MovesInfo pseudo_legal_moves{ board.pseudo_legal_move_gen(colour) };
                            for (int i{}; i < pseudo_legal_moves.count; i++)
                            {
                                engine::Move& move{ pseudo_legal_moves.moves[i] };
                                if (string_to_index[start_square] == move.start_square && string_to_index[destination_square] == move.destination_square && string_to_piece[promotion_piece] == move.promotion_piece)
                                {
                                    board.make_move(move);
                                    colour = -colour;
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (word == "go")
                {
                    int default_depth{ 7 };
                    std::string sub_word{};

                    while (input_stream >> sub_word)
                    {
                        if (sub_word == "depth")
                        {
                            input_stream >> default_depth;
                        }

                        // And we'll also have to add other things here for time management
                    }

                    engine::BestMove best_move{ search.get_best_move(default_depth, colour) };
                    std::string best_move_string
                    {
                        index_to_string[best_move.move.start_square] 
                        + index_to_string[best_move.move.destination_square]
                    };

                    if (best_move.move.promotion_piece != 0)
                    {
                        best_move_string += piece_to_string[best_move.move.promotion_piece];
                    }

                    std::cout << "bestmove " << best_move_string << std::endl;
                    std::cout << std::flush;
                }
                else if (word == "quit")
                {
                    break;
                }
            }
        }
    }

    return 0;
}

// g++ -O3 test.cpp -o test

// Up to depth 8 from the starting position is it right
// 84998978956 nodes in 2604.53 seconds.
// NPS: 32635008