#include <array>
#include <iostream>
#include <unordered_map>
#include <cctype>

namespace engine
{
    namespace Piece
    {
        static constexpr int white_pawn{ 1 };
        static constexpr int white_knight{ 2 };
        static constexpr int white_bishop{ 3 };
        static constexpr int white_rook{ 4 };
        static constexpr int white_queen{ 5 };
        static constexpr int white_king{ 6 };

        static constexpr int black_pawn{ -1 };
        static constexpr int black_knight{ -2 };
        static constexpr int black_bishop{ -3 };
        static constexpr int black_rook{ -4 };
        static constexpr int black_queen{ -5 };
        static constexpr int black_king{ -6 };

        static constexpr int empty{ 0 };
    }

    namespace BoardPart
    {
        static constexpr int start_index{ 26 };
        static constexpr int end_index{ 117 };
        
        static constexpr int columns{ 12 };
        static constexpr int rows{ 12 };

        static constexpr int padding{ -9 };
    }

    namespace Colour
    {
        static constexpr int white{ 1 };
        static constexpr int black{ -1 };
    }

    struct Move
    {
        int start_square;
        int destination_square;
        int promotion_piece;
    };

    struct MovesInfo
    {
        std::array<Move,218> moves{};
        int count;
    };

    class Board
    {
        private:
        std::array<int, 144> board{};
        std::unordered_map<int, char> id_to_piece{};

        public:
        Board()
        {
            static constexpr std::array<char, 7> pieces
            {
                ' ', 'p', 'n', 'b', 'r', 'q', 'k',
            };

            for (int i{}; i < 2; i++)
            {
                for (size_t j{}; j < pieces.size(); j++)
                {
                    if (i == 0)
                    {
                        id_to_piece[j] = (char)std::toupper(pieces[j]);
                    }
                    else if (i == 1)
                    {
                        id_to_piece[-j] = pieces[j];
                    }
                }
            }

            board.fill(-9);

            static constexpr std::array<int, 8> start_row
            {
                Piece::white_rook, Piece::white_knight, Piece::white_bishop, Piece::white_queen,
                Piece::white_king, Piece::white_bishop, Piece::white_knight, Piece::white_rook,
            };

            // Board setup
            for (int i{ BoardPart::start_index }; i <= BoardPart::end_index; i++)
            {
                int current_column{ i % BoardPart::columns };
                if (current_column > 1 && current_column < 10) // Ensure it doesn't write to the padding
                {
                    int current_row{ i / BoardPart::rows };
                    if (current_row == 2)
                    {
                        board[i] = -start_row[i - BoardPart::start_index];
                    }
                    else if (current_row == 3)
                    {
                        board[i] = Piece::black_pawn;
                    }
                    else if (current_row == 8)
                    {
                        board[i] = Piece::white_pawn;
                    }
                    else if (current_row == 9)
                    {
                        board[i] = start_row[i - 110];
                    }
                    else
                    {
                        board[i] = Piece::empty;
                    }
                }
            }
        }

        void print_board()
        {
            std::cout << "+---+---+---+---+---+---+---+---+" << '\n';
            for (int i{ BoardPart::start_index }; i <= BoardPart::end_index; i++)
            {
                int current_column{ i % BoardPart::columns };
                if (current_column > 1 && current_column < 10) // Ensure it doesn't output the padding
                {
                    if (current_column == 2)
                    {
                        std::cout << "| ";
                    }

                    std::cout << id_to_piece[board[i]] << " | ";
                    if (current_column == 9)
                    {
                        std::cout << '\n' << "+---+---+---+---+---+---+---+---+" << '\n';
                    }
                }
            }
        }

        MovesInfo pseudo_legal_move_gen(int colour) // 1 = white, -1 = black
        {
            std::array<Move, 218> pseudo_legal_moves{};
            int counter{}; // To keep track of which index we're on in the move list

            static constexpr std::array<int,8> knight_offsets{ +25, +23, +14, +10, -10, -14, -23, -25};//knight offsets
            static constexpr std::array<int,8> king_offsets{ +1, +13, +12, +11, -1, -13, -12, -11};
            static constexpr std::array<int, 4> bishop_offsets{ 13, 11, -11, -13 };
            static constexpr std::array<int, 4> rook_offsets{ 12, 1, -1, -12 };
            for (int i{}; i < board.size(); i++)
            {
                // And now we just check every square and if it's x piece, apply x's movement
                int piece = board[i];
                if (piece == BoardPart::padding || piece == Piece::empty) continue;
                if (piece * colour < 0) continue; // If the current piece isn't of the colour we're searching for

                // knight moves
                if(piece == Piece::white_knight * colour){
                    for(int offset : knight_offsets){
                        int target_loc = i + offset;
                        if(board[target_loc] == BoardPart::padding) continue; // padding encountered
                        if(board[target_loc]*piece <= 0){ //opponent capture means target and current pieces of different signs
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0};
                        }
                    }
                }

                //king moves
                if(piece == Piece::white_king * colour){
                    for(int offset : king_offsets){
                        int target_loc = i + offset;
                        if(board[target_loc] == BoardPart::padding) continue; // padding encountered
                        if(board[target_loc]*piece <= 0){ //opponent capture means target and current pieces of different signs
                            // rules like castling not implemented yet
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0};
                        }
                    }
                }

                // Bishop moves and diagonal queen moves
                if (piece == Piece::white_bishop * colour || piece == Piece::white_queen * colour)
                {
                    for (int j{}; j < bishop_offsets.size(); j++)
                    {
                        int target_loc{ i + bishop_offsets[j] };
                        while (board[target_loc] * piece <= 0 && board[target_loc] != BoardPart::padding)
                        {
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0};
                            if (board[target_loc] != Piece::empty) // Stop if it hit a piece
                            {
                                break;
                            }

                            target_loc += bishop_offsets[j];
                        }
                    }
                }

                // Rook moves and horizontal and vertical queen moves
                if (piece == Piece::white_rook * colour || piece == Piece::white_queen * colour)
                {
                    for (int j{}; j < rook_offsets.size(); j++)
                    {
                        int target_loc{ i + rook_offsets[j] };
                        while (board[target_loc] * piece <= 0 && board[target_loc] != BoardPart::padding)
                        {
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0};
                            if (board[target_loc] != Piece::empty) // Stop if it hit a piece
                            {
                                break;
                            }

                            target_loc += rook_offsets[j];
                        }
                    }
                }
            }
            
            return MovesInfo{pseudo_legal_moves,counter};
        }
    };
}