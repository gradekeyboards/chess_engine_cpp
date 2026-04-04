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
    }

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
    };
}