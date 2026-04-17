#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <array>
#include <iostream>
#include <unordered_map>
#include <cctype>
#include <algorithm>
#include <random>

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

    namespace Zobrist
    {
        static constexpr int white_kingside_castle{ 0 };
        static constexpr int white_queenside_castle{ 1 };
        static constexpr int black_kingside_castle { 2 };
        static constexpr int black_queenside_castle{ 3 };

        static constexpr int white{ 1 };
        static constexpr int black{ 0 };
    }

    enum class MoveFlag
    {
        capture = 1,
        quiet = 2,
        double_push = 3,
        en_passant = 4,
        castle = 5,
        promotion = 6,
        promotion_capture = 7,
    };

    struct Move
    {
        int start_square;
        int destination_square;
        int promotion_piece;
        MoveFlag flag;
        int score;
    };

    struct MovesInfo
    {
        std::array<Move,218> moves{};
        int count;
    };

    // For make_move() and undo_move()
    struct MoveHistory
    {
        int captured_piece;
        int colour;
        bool white_kingside_castle;
        bool white_queenside_castle;
        bool black_kingside_castle;
        bool black_queenside_castle;
        int en_passant_square;
        int current_piece_material;
        int mg_PST_evaluation;
        int eg_PST_evaluation;
        int white_material;
        int black_material;
        uint64_t current_zobrist_position;
    };

    // For some reason it wouldn't work if it was in the class?
    static constexpr std::array<std::array<int, 144>, 7> generate_black_PSTs(const std::array<std::array<int, 144>, 7>& mg_white_PSTs)
    {
        std::array<std::array<int, 144>, 7> mg_black_PSTs{};
        for (int i{}; i < mg_white_PSTs.size(); i++)
        {
            const std::array<int, 144>& PST{ mg_white_PSTs[i] };
            for (int j{}; j < PST.size(); j++)
            {
                int value{ PST[j] };
                int white_index{ j };
                int black_index{ j + 12 * (11 - 2 * (white_index / 12)) };

                mg_black_PSTs[i][black_index] = value;
            }
        }
        return mg_black_PSTs;
    }

    class Board
    {
        private:
        std::array<int, 144> board{};
        std::unordered_map<int, char> id_to_piece{};

        static constexpr std::array<int,8> knight_offsets{ +25, +23, +14, +10, -10, -14, -23, -25};//knight offsets
        static constexpr std::array<int,8> king_offsets{ +1, +13, +12, +11, -1, -13, -12, -11};
        static constexpr std::array<int, 4> bishop_offsets{ 13, 11, -11, -13 };
        static constexpr std::array<int, 4> rook_offsets{ 12, 1, -1, -12 };

        bool white_kingside_castle = true;
        bool white_queenside_castle = true;
        bool black_kingside_castle = true;
        bool black_queenside_castle = true;

        //used to check castling condition in make_move function
        static constexpr int white_kingside_rook_starting_square = BoardPart::end_index;
        static constexpr int white_queenside_rook_starting_square = 110;
        static constexpr int black_kingside_rook_starting_square = 33;
        static constexpr int black_queenside_rook_starting_square = BoardPart::start_index;

        // To make checking if the kings are in check easier
        int white_king_position{ 114 };
        int black_king_position{ 30 };

        int white_king_start_square{ 114 };
        int black_king_start_square{ 30 };

        int en_passant_square = -1;

        // Evaluation variables
        int mg_PST_evaluation{};
        int eg_PST_evaluation{};

        int white_material{ piece_values[Piece::white_pawn] * 8 + piece_values[Piece::white_knight] * 2 + piece_values[Piece::white_bishop] * 2 + piece_values[Piece::white_rook] * 2 + piece_values[Piece::white_queen] + piece_values[Piece::white_king] };
        int black_material{ piece_values[Piece::white_pawn] * 8 + piece_values[Piece::white_knight] * 2 + piece_values[Piece::white_bishop] * 2 + piece_values[Piece::white_rook] * 2 + piece_values[Piece::white_queen] + piece_values[Piece::white_king] };

        // We can change these later
        static constexpr std::array<int, 7> piece_values
        {
            // padding, P, N, B, R, Q, K
            0, 100, 300, 350, 500, 1000, 10000
        };

        static constexpr int start_piece_material{ piece_values[Piece::white_knight] * 4 + piece_values[Piece::white_bishop] * 4 + piece_values[Piece::white_rook] * 4 + piece_values[Piece::white_queen] * 2 };
        int current_piece_material{ piece_values[Piece::white_knight] * 4 + piece_values[Piece::white_bishop] * 4 + piece_values[Piece::white_rook] * 4 + piece_values[Piece::white_queen] * 2 };

        static constexpr std::array<std::array<int, 144>, 7> mg_white_PSTs
        {{
            // Padding
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Pawns
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,  98, 134,  61,  95,  68, 126,  34, -11,   0,   0, 
                0,   0,  -6,   7,  26,  31,  65,  56,  25, -20,   0,   0, 
                0,   0, -14,  13,   6,  21,  23,  12,  17, -23,   0,   0, 
                0,   0, -27,  -2,  -5,  12,  17,   6,  10, -25,   0,   0, 
                0,   0, -26,  -4,  -4, -10,   3,   3,  33, -12,   0,   0, 
                0,   0, -35,  -1, -20, -23, -15,  24,  38, -22,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Knights
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0, -167, -89, -34, -49,  61, -97, -15, -107,   0,   0, 
                0,   0, -73, -41,  72,  36,  23,  62,   7, -17,   0,   0, 
                0,   0, -47,  60,  37,  65,  84, 129,  73,  44,   0,   0, 
                0,   0,  -9,  17,  19,  53,  37,  69,  18,  22,   0,   0, 
                0,   0, -13,   4,  16,  13,  28,  19,  21,  -8,   0,   0, 
                0,   0, -23,  -9,  12,  10,  19,  17,  25, -16,   0,   0, 
                0,   0, -29, -53, -12,  -3,  -1,  18, -14, -19,   0,   0, 
                0,   0, -105, -21, -58, -33, -17, -28, -19, -23,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Bishops
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0, -29,   4, -82, -37, -25, -42,   7,  -8,   0,   0, 
                0,   0, -26,  16, -18, -13,  30,  59,  18, -47,   0,   0, 
                0,   0, -16,  37,  43,  40,  35,  50,  37,  -2,   0,   0, 
                0,   0,  -4,   5,  19,  50,  37,  37,   7,  -2,   0,   0, 
                0,   0,  -6,  13,  13,  26,  34,  12,  10,   4,   0,   0, 
                0,   0,   0,  15,  15,  15,  14,  27,  18,  10,   0,   0, 
                0,   0,   4,  15,  16,   0,   7,  21,  33,   1,   0,   0, 
                0,   0, -33,  -3, -14, -21, -13, -12, -39, -21,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Rooks
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,  32,  42,  32,  51,  63,   9,  31,  43,   0,   0, 
                0,   0,  27,  32,  58,  62,  80,  67,  26,  44,   0,   0, 
                0,   0,  -5,  19,  26,  36,  17,  45,  61,  16,   0,   0, 
                0,   0, -24, -11,   7,  26,  24,  35,  -8, -20,   0,   0, 
                0,   0, -36, -26, -12,  -1,   9,  -7,   6, -23,   0,   0, 
                0,   0, -45, -25, -16, -17,   3,   0,  -5, -33,   0,   0, 
                0,   0, -44, -16, -20,  -9,  -1,  11,  -6, -71,   0,   0, 
                0,   0, -19, -13,   1,  17,  16,   7, -37, -26,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Queens
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0, -28,   0,  29,  12,  59,  44,  43,  45,   0,   0, 
                0,   0, -24, -39,  -5,   1, -16,  57,  28,  54,   0,   0, 
                0,   0, -13, -17,   7,   8,  29,  56,  47,  57,   0,   0, 
                0,   0, -27, -27, -16, -16,  -1,  17,  -2,   1,   0,   0, 
                0,   0,  -9, -26,  -9, -10,  -2,  -4,   3,  -3,   0,   0, 
                0,   0, -14,   2, -11,  -2,  -5,   2,  14,   5,   0,   0, 
                0,   0, -35,  -8,  11,   2,   8,  15,  -3,   1,   0,   0, 
                0,   0,  -1, -18,  -9,  10, -15, -25, -31, -50,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Kings
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0, -65,  23,  16, -15, -56, -34,   2,  13,   0,   0, 
                0,   0,  29,  -1, -20,  -7,  -8,  -4, -38, -29,   0,   0, 
                0,   0,  -9,  24,   2, -16, -20,   6,  22, -22,   0,   0, 
                0,   0, -17, -20, -12, -27, -30, -25, -14, -36,   0,   0, 
                0,   0, -49,  -1, -27, -39, -46, -44, -33, -51,   0,   0, 
                0,   0, -14, -14, -22, -46, -44, -30, -15, -27,   0,   0, 
                0,   0,   1,   7,  -8, -64, -43, -16,   9,   8,   0,   0, 
                0,   0, -15,  36,  12, -54,   8, -28,  24,  14,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },
        }};

        static constexpr std::array<std::array<int, 144>, 7> eg_white_PSTs
        {{
            // Padding
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Pawns,
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0, 178, 173, 158, 134, 147, 132, 165, 187,   0,   0, 
                0,   0,  94, 100,  85,  67,  56,  53,  82,  84,   0,   0, 
                0,   0,  32,  24,  13,   5,  -2,   4,  17,  17,   0,   0, 
                0,   0,  13,   9,  -3,  -7,  -7,  -8,   3,  -1,   0,   0, 
                0,   0,   4,   7,  -6,   1,   0,  -5,  -1,  -8,   0,   0, 
                0,   0,  13,   8,   8,  10,  13,   0,   2,  -7,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Knights
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0, -58, -38, -13, -28, -31, -27, -63, -99,   0,   0, 
                0,   0, -25,  -8, -25,  -2,  -9, -25, -24, -52,   0,   0, 
                0,   0, -24, -20,  10,   9,  -1,  -9, -19, -41,   0,   0, 
                0,   0, -17,   3,  22,  22,  22,  11,   8, -18,   0,   0, 
                0,   0, -18,  -6,  16,  25,  16,  17,   4, -18,   0,   0, 
                0,   0, -23,  -3,  -1,  15,  10,  -3, -20, -22,   0,   0, 
                0,   0, -42, -20, -10,  -5,  -2, -20, -23, -44,   0,   0, 
                0,   0, -29, -51, -23, -15, -22, -18, -50, -64,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Bishops
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0, -14, -21, -11,  -8,  -7,  -9, -17, -24,   0,   0, 
                0,   0,  -8,  -4,   7, -12,  -3, -13,  -4, -14,   0,   0, 
                0,   0,   2,  -8,   0,  -1,  -2,   6,   0,   4,   0,   0, 
                0,   0,  -3,   9,  12,   9,  14,  10,   3,   2,   0,   0, 
                0,   0,  -6,   3,  13,  19,   7,  10,  -3,  -9,   0,   0, 
                0,   0, -12,  -3,   8,  10,  13,   3,  -7, -15,   0,   0, 
                0,   0, -14, -18,  -7,  -1,   4,  -9, -15, -27,   0,   0, 
                0,   0, -23,  -9, -23,  -5,  -9, -16,  -5, -17,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Rooks
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,  13,  10,  18,  15,  12,  12,   8,   5,   0,   0, 
                0,   0,  11,  13,  13,  11,  -3,   3,   8,   3,   0,   0, 
                0,   0,   7,   7,   7,   5,   4,  -3,  -5,  -3,   0,   0, 
                0,   0,   4,   3,  13,   1,   2,   1,  -1,   2,   0,   0, 
                0,   0,   3,   5,   8,   4,  -5,  -6,  -8, -11,   0,   0, 
                0,   0,  -4,   0,  -5,  -1,  -7, -12,  -8, -16,   0,   0, 
                0,   0,  -6,  -6,   0,   2,  -9,  -9, -11,  -3,   0,   0, 
                0,   0,  -9,   2,   3,  -1,  -5, -13,   4, -20,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Queens
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,  -9,  22,  22,  27,  27,  19,  10,  20,   0,   0, 
                0,   0, -17,  20,  32,  41,  58,  25,  30,   0,   0,   0, 
                0,   0, -20,   6,   9,  49,  47,  35,  19,   9,   0,   0, 
                0,   0,   3,  22,  24,  45,  57,  40,  57,  36,   0,   0, 
                0,   0, -18,  28,  19,  47,  31,  34,  39,  23,   0,   0, 
                0,   0, -16, -27,  15,   6,   9,  17,  10,   5,   0,   0, 
                0,   0, -22, -23, -30, -16, -16, -23, -36, -32,   0,   0, 
                0,   0, -33, -28, -22, -43,  -5, -32, -20, -41,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },

            // Kings
            {
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0, -74, -35, -18, -18, -11,  15,   4, -17,   0,   0, 
                0,   0, -12,  17,  14,  17,  17,  38,  23,  11,   0,   0, 
                0,   0,  10,  17,  23,  15,  20,  45,  44,  13,   0,   0, 
                0,   0,  -8,  22,  24,  27,  26,  33,  26,   3,   0,   0, 
                0,   0, -18,  -4,  21,  24,  27,  23,   9, -11,   0,   0, 
                0,   0, -19,  -3,  11,  21,  23,  16,   7,  -9,   0,   0, 
                0,   0, -27, -11,   4,  13,  14,   4,  -5, -17,   0,   0, 
                0,   0, -53, -34, -21, -11, -28, -14, -24, -43,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
                0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            }
        }};

        static constexpr std::array<std::array<int, 144>, 7> mg_black_PSTs = generate_black_PSTs(mg_white_PSTs);
        static constexpr std::array<std::array<int, 144>, 7> eg_black_PSTs = generate_black_PSTs(eg_white_PSTs);

        // Zobrist keys
        // 2 = black and white, 6 + 1 = the number of piece types + 1 for padding (so that 1 = pawn, 2 = knight...), 144 = board size
        std::array<std::array<std::array<uint64_t, 144>, 7>, 2> zobrist_piece_keys{};
        std::array<uint64_t, 4> zobrist_castle_right_keys{};
        std::array<uint64_t, 8> zobrist_en_passant_keys{};
        uint64_t zobrist_white_to_move{}; // If this is in current_zobrist_position, that means it's white's turn

        // To check for threefold repetition and 50-move rule draws
        std::vector<uint64_t> position_history{};
        uint64_t current_zobrist_position{};

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

            // Zobrist keys setup
            std::random_device rand_dev{};
            std::mt19937_64 gen{ rand_dev() };
            std::uniform_int_distribution<uint64_t> distrib{};

            for (int i{}; i < zobrist_piece_keys.size(); i++)
            {
                auto& colour{ zobrist_piece_keys[i] };
                for (int j{}; j < colour.size(); j++)
                {
                    auto& piece_type{ colour[j] };
                    for (int k{}; k < piece_type.size(); k++)
                    {
                        if (j == 0)
                        {
                            zobrist_piece_keys[i][j][k] = 0;
                        }
                        else
                        {
                            zobrist_piece_keys[i][j][k] = distrib(gen);
                        }
                    }
                }
            }

            for (auto& column : zobrist_en_passant_keys)
            {
                column = distrib(gen);
            }

            for (auto& castle_right : zobrist_castle_right_keys)
            {
                castle_right = distrib(gen);
                current_zobrist_position ^= castle_right;
            }

            zobrist_white_to_move = distrib(gen);
            current_zobrist_position ^= zobrist_white_to_move;

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

                    int piece{ board[i] };
                    int piece_type{ std::abs(piece) };

                    if (piece > 0)
                    {
                        mg_PST_evaluation += mg_white_PSTs[piece_type][i];
                        eg_PST_evaluation += eg_white_PSTs[piece_type][i];
                    }
                    else if (piece < 0)
                    {
                        mg_PST_evaluation -= mg_black_PSTs[piece_type][i];
                        eg_PST_evaluation -= eg_black_PSTs[piece_type][i];
                    }

                    // Setup the initial zobrist position
                    int zobrist_colour = (piece > 0) ? 1 : 0;
                    current_zobrist_position ^= zobrist_piece_keys[zobrist_colour][piece_type][i];
                }
            }

            position_history.push_back(current_zobrist_position); // The initial position because techically the players could move their knights back 
                                                                  // and forth to get the same position many times and cause threefold repetition
        }

        friend class Search;
        friend class Eval;

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

        // This function and the other should probably return std::array<int, 144>
        static std::unordered_map<int, std::string> get_index_to_string()
        {
            std::unordered_map<int, std::string> index_to_string{};
            for (int i{ BoardPart::start_index }; i <= BoardPart::end_index; i++)
            {
                std::string square{};
                int current_column{ i % BoardPart::columns };
                int current_row{ i / BoardPart::rows };
                if (current_column > 1 && current_column < 10) // Ensure it doesn't write to the padding
                {
                    switch (current_column)
                    {
                        case 2:
                            square += "a";
                            break;
                        case 3:
                            square += "b";
                            break;
                        case 4:
                            square += "c";
                            break;
                        case 5:
                            square += "d";
                            break;
                        case 6:
                            square += "e";
                            break;
                        case 7:
                            square += "f";
                            break;
                        case 8:
                            square += "g";
                            break;
                        case 9:
                            square += "h";
                            break;
                    }

                    switch (current_row)
                    {
                        case 2:
                            square += "8";
                            break;
                        case 3:
                            square += "7";
                            break;
                        case 4:
                            square += "6";
                            break;
                        case 5:
                            square += "5";
                            break;
                        case 6:
                            square += "4";
                            break;
                        case 7:
                            square += "3";
                            break;
                        case 8:
                            square += "2";
                            break;
                        case 9:
                            square += "1";
                            break;
                    }

                    index_to_string[i] = square;
                }
            }
            return index_to_string;
        }

        static std::unordered_map<std::string, int> get_string_to_index()
        {
            std::unordered_map<std::string, int> string_to_index{};
            for (const auto& [index, string] : get_index_to_string())
            {
                string_to_index[string] = index;
            }
            return string_to_index;
        }

        bool is_square_attacked(int square_index, int defending_colour)
        {
            int colour = defending_colour;

            int forward = -(colour)*BoardPart::columns; // for white , forward = upwards(-12) and for black forwar = downward(+12)
            int one_step = square_index + forward;

            int left_capture = one_step - 1;
            int right_capture = one_step + 1;

            // Pawn attacks
            if (board[left_capture] * colour == Piece::black_pawn || board[right_capture] * colour == Piece::black_pawn)
            {
                return true;
            }            

            // Knight attacks
            for (int offset : knight_offsets)
            {
                int target_loc{ square_index + offset };
                if (board[target_loc] * colour == Piece::black_knight)
                {
                    return true;
                }
            }

            // King attacks
            for (int offset : king_offsets)
            {
                int target_loc{ square_index + offset };
                if (board[target_loc] * colour == Piece::black_king)
                {
                    return true;
                }
            }

            // Bishop and diagonal queen attacks
            for (int i{}; i < bishop_offsets.size(); i++)
            {
                int target_loc{ square_index + bishop_offsets[i] };
                while (board[target_loc] != BoardPart::padding)
                {
                    if (board[target_loc] != Piece::empty)
                    {
                        if (board[target_loc] * colour == Piece::black_bishop || board[target_loc] * colour == Piece::black_queen)
                        {
                            return true;
                        }

                        // If it is a friendly piece then break
                        break;
                    }

                    target_loc += bishop_offsets[i];
                }
            }

            // Rook and horizontal and vertical queen attacks
            for (int i{}; i < rook_offsets.size(); i++)
            {
                int target_loc{ square_index + rook_offsets[i] };
                while (board[target_loc] != BoardPart::padding)
                {
                    if (board[target_loc] != Piece::empty)
                    {
                        if (board[target_loc] * colour == Piece::black_rook || board[target_loc] * colour == Piece::black_queen)
                        {
                            return true;
                        }

                        break;
                    }

                    target_loc += rook_offsets[i];
                }
            }

            return false;
        }

        MovesInfo pseudo_legal_move_gen(int colour) // 1 = white, -1 = black
        {
            std::array<Move, 218> pseudo_legal_moves{};
            int counter{}; // To keep track of which index we're on in the move list

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
                        if(board[target_loc]*piece < 0){ //opponent capture means target and current pieces of different signs
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0, MoveFlag::capture};
                        }
                        else if (board[target_loc] == Piece::empty)
                        {
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0, MoveFlag::quiet};
                        }
                    }
                }

                //king moves
                if(piece == Piece::white_king * colour){
                    for(int offset : king_offsets){
                        int target_loc = i + offset;
                        if(board[target_loc] == BoardPart::padding) continue; // padding encountered
                        if(board[target_loc]*piece < 0){ //opponent capture means target and current pieces of different signs
                            // rules like castling not implemented yet
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0, MoveFlag::capture};
                        }
                        else if (board[target_loc] == Piece::empty)
                        {
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0, MoveFlag::quiet};
                        }
                    }

                    //castling
                    if(colour == Colour::white && white_kingside_castle == true){ //right side white
                        // Maybe we should make these for loops at some point?
                        if(!is_square_attacked(i, colour) && board[i+1] == Piece::empty && !is_square_attacked(i + 1, colour) && board[i+2] == Piece::empty && !is_square_attacked(i + 2, colour) && board[i+3] == Piece::white_rook){
                            pseudo_legal_moves[counter++] = Move{i, i+2, 0, MoveFlag::castle};
                        }
                    }
                    if(colour == Colour::black && black_kingside_castle == true){ //right side black
                        if(!is_square_attacked(i, colour) && board[i+1] == Piece::empty && !is_square_attacked(i + 1, colour) && board[i+2] == Piece::empty && !is_square_attacked(i + 2, colour) && board[i+3] == Piece::black_rook){
                            pseudo_legal_moves[counter++] = Move{i, i+2, 0, MoveFlag::castle};
                        }
                    }
                    if(colour == Colour::white && white_queenside_castle == true){ //left side white
                        if(!is_square_attacked(i, colour) && board[i-1] == Piece::empty && !is_square_attacked(i - 1, colour) && board[i-2] == Piece::empty && !is_square_attacked(i - 2, colour) && board[i-3] == Piece::empty && board[i-4] == Piece::white_rook){
                            pseudo_legal_moves[counter++] = Move{i, i-2, 0, MoveFlag::castle};
                        }
                    }
                    if(colour == Colour::black && black_queenside_castle == true){ //left side black
                        if(!is_square_attacked(i, colour) && board[i-1] == Piece::empty && !is_square_attacked(i - 1, colour) && board[i-2] == Piece::empty && !is_square_attacked(i - 2, colour) && board[i-3] == Piece::empty && board[i-4] == Piece::black_rook){
                            pseudo_legal_moves[counter++] = Move{i, i-2, 0, MoveFlag::castle};
                        }
                    }
                }

                //pawn moves
                if(piece == Piece::white_pawn * colour){
                    int forward = -(colour)*BoardPart::columns; // for white , forward = upwards(-12) and for black forwar = downward(+12)
                    int one_step = i + forward;
                    int two_steps = i + 2*forward;

                    int left_capture = one_step - 1;
                    int right_capture = one_step + 1;

                    int current_row = i / BoardPart::columns;
                    int target_row = one_step / BoardPart::columns;

                    int starting_row = 0;
                    int promotion_row = 0;
                    if(colour == 1){
                        starting_row = 8;
                        promotion_row = 2;
                    }
                    else{
                        starting_row = 3;
                        promotion_row = 9;
                    }

                    //single move
                    if(board[one_step] == Piece::empty){
                        //check promotion
                        if(target_row == promotion_row){
                            pseudo_legal_moves[counter++] = Move{i, one_step, Piece::white_queen*colour, MoveFlag::promotion};
                            pseudo_legal_moves[counter++] = Move{i, one_step, Piece::white_rook*colour, MoveFlag::promotion};
                            pseudo_legal_moves[counter++] = Move{i, one_step, Piece::white_knight*colour, MoveFlag::promotion};
                            pseudo_legal_moves[counter++] = Move{i, one_step, Piece::white_bishop*colour, MoveFlag::promotion};
                        }
                        else{
                            pseudo_legal_moves[counter++] = Move{i, one_step, 0, MoveFlag::quiet}; // no promotion, only 1 step forward
                        }

                        //double move
                        if(current_row == starting_row && board[two_steps] == Piece::empty){
                            pseudo_legal_moves[counter++] = Move{i, two_steps, 0, MoveFlag::double_push};
                        }
                    }

                    //left capture
                    if(board[left_capture] != BoardPart::padding && board[left_capture]*piece < 0){ // avoid padding and target location has opponent piece only
                        target_row = left_capture / BoardPart::columns;
                        if(target_row == promotion_row){
                            pseudo_legal_moves[counter++] = Move{i, left_capture, Piece::white_queen*colour, MoveFlag::promotion_capture};
                            pseudo_legal_moves[counter++] = Move{i, left_capture, Piece::white_rook*colour, MoveFlag::promotion_capture};
                            pseudo_legal_moves[counter++] = Move{i, left_capture, Piece::white_knight*colour, MoveFlag::promotion_capture};
                            pseudo_legal_moves[counter++] = Move{i, left_capture, Piece::white_bishop*colour, MoveFlag::promotion_capture};
                        }
                        else{
                            pseudo_legal_moves[counter++] = Move{i, left_capture, 0, MoveFlag::capture};
                        }
                    }

                    //right capture
                    if(board[right_capture] != BoardPart::padding && board[right_capture]*piece < 0){ // avoid padding and target location has opponent piece only
                        target_row = right_capture / BoardPart::columns;
                        if(target_row == promotion_row){
                            pseudo_legal_moves[counter++] = Move{i, right_capture, Piece::white_queen*colour, MoveFlag::promotion_capture};
                            pseudo_legal_moves[counter++] = Move{i, right_capture, Piece::white_rook*colour, MoveFlag::promotion_capture};
                            pseudo_legal_moves[counter++] = Move{i, right_capture, Piece::white_knight*colour, MoveFlag::promotion_capture};
                            pseudo_legal_moves[counter++] = Move{i, right_capture, Piece::white_bishop*colour, MoveFlag::promotion_capture};
                        }
                        else{
                            pseudo_legal_moves[counter++] = Move{i, right_capture, 0, MoveFlag::capture};
                        }
                    }

                    //en passant
                    if(left_capture == en_passant_square){
                        pseudo_legal_moves[counter++] = Move{i, left_capture, 0, MoveFlag::en_passant};
                    }
                    if(right_capture == en_passant_square){
                        pseudo_legal_moves[counter++] = Move{i, right_capture, 0, MoveFlag::en_passant};
                    }

                    //later will write a function to reduce redundancy
                    //might as well edit struct Move and add enum MoveFlag to store movetype(capture, quiet, en passant, castle,promotion, promotion-capture ,etc)
                    //after writing enum MoveFlag , update each move with appropriate flag: quiet,capture,etc.
                }

                // Bishop moves and diagonal queen moves
                if (piece == Piece::white_bishop * colour || piece == Piece::white_queen * colour)
                {
                    for (int j{}; j < bishop_offsets.size(); j++)
                    {
                        int target_loc{ i + bishop_offsets[j] };
                        while (board[target_loc] * piece <= 0 && board[target_loc] != BoardPart::padding)
                        {
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0, MoveFlag::quiet};
                            if (board[target_loc] != Piece::empty) // Stop if it hit a piece
                            {
                                if (board[target_loc] * piece < 0)
                                {
                                    pseudo_legal_moves[counter - 1].flag = MoveFlag::capture;
                                }

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
                            pseudo_legal_moves[counter++] = Move{i, target_loc, 0, MoveFlag::quiet};
                            if (board[target_loc] != Piece::empty) // Stop if it hit a piece
                            {
                                if (board[target_loc] * piece < 0)
                                {
                                    pseudo_legal_moves[counter - 1].flag = MoveFlag::capture;
                                }

                                break;
                            }

                            target_loc += rook_offsets[j];
                        }
                    }
                }
            }
            
            return MovesInfo{pseudo_legal_moves,counter};
        }

        MoveHistory make_move(const Move& move){
            MoveHistory history{};
            history.current_piece_material = current_piece_material;
            history.mg_PST_evaluation = mg_PST_evaluation;
            history.eg_PST_evaluation = eg_PST_evaluation;
            history.white_material = white_material;
            history.black_material = black_material;
            history.current_zobrist_position = current_zobrist_position;

            int piece = board[move.start_square];
            int piece_type{ std::abs(piece) };
            int promotion_piece_type{ std::abs(move.promotion_piece) };
            int captured_piece = board[move.destination_square]; //helpful for updating castling conditions
            int captured_piece_type{ std::abs(captured_piece) };
            board[move.destination_square] = piece;
            board[move.start_square] = Piece::empty;

            // Update the evaluations
            if (move.flag == MoveFlag::capture || move.flag == MoveFlag::promotion_capture)
            {
                if (captured_piece > 0)
                {
                    white_material -= piece_values[captured_piece_type];
                }
                else if (captured_piece < 0)
                {
                    black_material -= piece_values[captured_piece_type];
                }

                if (captured_piece_type != Piece::white_pawn)
                {
                    current_piece_material -= piece_values[captured_piece_type];
                }
            }

            if (piece > 0)
            {
                // Minus to remove a white piece
                mg_PST_evaluation -= mg_white_PSTs[piece_type][move.start_square];
                eg_PST_evaluation -= eg_white_PSTs[piece_type][move.start_square];

                current_zobrist_position ^= zobrist_piece_keys[Zobrist::white][piece_type][move.start_square];

                // Plus to remove a black piece
                mg_PST_evaluation += mg_black_PSTs[captured_piece_type][move.destination_square];
                eg_PST_evaluation += eg_black_PSTs[captured_piece_type][move.destination_square];

                current_zobrist_position ^= zobrist_piece_keys[Zobrist::black][captured_piece_type][move.destination_square];

                if (move.promotion_piece == 0)
                {
                    // Plus to add a white piece
                    mg_PST_evaluation += mg_white_PSTs[piece_type][move.destination_square];
                    eg_PST_evaluation += eg_white_PSTs[piece_type][move.destination_square];

                    current_zobrist_position ^= zobrist_piece_keys[Zobrist::white][piece_type][move.destination_square];
                }
                else
                {
                    mg_PST_evaluation += mg_white_PSTs[promotion_piece_type][move.destination_square];
                    eg_PST_evaluation += eg_white_PSTs[promotion_piece_type][move.destination_square];

                    current_zobrist_position ^= zobrist_piece_keys[Zobrist::white][promotion_piece_type][move.destination_square];
                }
            }
            else
            {
                // Plus to remove a black piece
                mg_PST_evaluation += mg_black_PSTs[piece_type][move.start_square];
                eg_PST_evaluation += eg_black_PSTs[piece_type][move.start_square];

                current_zobrist_position ^= zobrist_piece_keys[Zobrist::black][piece_type][move.start_square];

                // Minus to remove a white piece
                mg_PST_evaluation -= mg_white_PSTs[captured_piece_type][move.destination_square];
                eg_PST_evaluation -= eg_white_PSTs[captured_piece_type][move.destination_square];

                current_zobrist_position ^= zobrist_piece_keys[Zobrist::white][captured_piece_type][move.destination_square];

                if (move.promotion_piece == 0)
                {
                    // Minus to add a black piece
                    mg_PST_evaluation -= mg_black_PSTs[piece_type][move.destination_square];
                    eg_PST_evaluation -= eg_black_PSTs[piece_type][move.destination_square];

                    current_zobrist_position ^= zobrist_piece_keys[Zobrist::black][piece_type][move.destination_square];
                }
                else
                {
                    mg_PST_evaluation -= mg_black_PSTs[promotion_piece_type][move.destination_square];
                    eg_PST_evaluation -= eg_black_PSTs[promotion_piece_type][move.destination_square];

                    current_zobrist_position ^= zobrist_piece_keys[Zobrist::black][promotion_piece_type][move.destination_square];
                }
            }

            history.captured_piece = captured_piece;
            history.colour = (piece > 0) ? Colour::white : Colour::black;
            history.white_kingside_castle = white_kingside_castle;
            history.white_queenside_castle = white_queenside_castle;
            history.black_kingside_castle = black_kingside_castle;
            history.black_queenside_castle = black_queenside_castle;
            history.en_passant_square = en_passant_square;
            
            //promotion
            if(move.promotion_piece != 0){
                board[move.destination_square] = move.promotion_piece;
                current_piece_material += piece_values[promotion_piece_type];

                if (move.promotion_piece > 0)
                {
                    white_material += piece_values[promotion_piece_type];
                    white_material -= piece_values[Piece::white_pawn];

                }
                else if (move.promotion_piece < 0)
                {
                    black_material += piece_values[promotion_piece_type];
                    black_material -= piece_values[Piece::white_pawn];
                }
            }

            // en passant
            if(move.flag == MoveFlag::en_passant){
                if(piece == Piece::white_pawn){
                    board[move.destination_square + BoardPart::columns] = Piece::empty;
                    mg_PST_evaluation += mg_black_PSTs[piece_type][move.destination_square + BoardPart::columns];
                    eg_PST_evaluation += eg_black_PSTs[piece_type][move.destination_square + BoardPart::columns];
                    black_material -= piece_values[piece_type];

                    current_zobrist_position ^= zobrist_piece_keys[Zobrist::black][piece_type][move.destination_square + BoardPart::columns];
                }
                else if(piece == Piece::black_pawn){
                    board[move.destination_square - BoardPart::columns] = Piece::empty;
                    mg_PST_evaluation -= mg_white_PSTs[piece_type][move.destination_square - BoardPart::columns];
                    eg_PST_evaluation -= eg_white_PSTs[piece_type][move.destination_square - BoardPart::columns];
                    white_material -= piece_values[piece_type];

                    current_zobrist_position ^= zobrist_piece_keys[Zobrist::white][piece_type][move.destination_square - BoardPart::columns];
                }
            }

            // //castling
            if(move.flag == MoveFlag::castle){
                //king already moved

                //kingside(rightside)
                if(move.destination_square > move.start_square){
                    if(piece == Piece::white_king){
                        board[move.destination_square - 1] = Piece::white_rook;
                        board[move.destination_square + 1] = Piece::empty;

                        mg_PST_evaluation -= mg_white_PSTs[Piece::white_rook][move.destination_square + 1];
                        eg_PST_evaluation -= eg_white_PSTs[Piece::white_rook][move.destination_square + 1];

                        mg_PST_evaluation += mg_white_PSTs[Piece::white_rook][move.destination_square - 1];
                        eg_PST_evaluation += eg_white_PSTs[Piece::white_rook][move.destination_square - 1];

                        current_zobrist_position ^= zobrist_piece_keys[Zobrist::white][Piece::white_rook][move.destination_square - 1];
                        current_zobrist_position ^= zobrist_piece_keys[Zobrist::white][Piece::white_rook][move.destination_square + 1];
                    }
                    else if(piece == Piece::black_king){
                        board[move.destination_square - 1] = Piece::black_rook;
                        board[move.destination_square + 1] = Piece::empty;

                        mg_PST_evaluation += mg_black_PSTs[Piece::white_rook][move.destination_square + 1];
                        eg_PST_evaluation += eg_black_PSTs[Piece::white_rook][move.destination_square + 1];

                        mg_PST_evaluation -= mg_black_PSTs[Piece::white_rook][move.destination_square - 1];
                        eg_PST_evaluation -= eg_black_PSTs[Piece::white_rook][move.destination_square - 1];

                        current_zobrist_position ^= zobrist_piece_keys[Zobrist::black][Piece::white_rook][move.destination_square - 1];
                        current_zobrist_position ^= zobrist_piece_keys[Zobrist::black][Piece::white_rook][move.destination_square + 1];
                    }
                }
                else if(move.destination_square < move.start_square){ //queenside(leftside)
                    if(piece == Piece::white_king){
                        board[move.destination_square + 1] = Piece::white_rook;
                        board[move.destination_square - 2] = Piece::empty;

                        mg_PST_evaluation -= mg_white_PSTs[Piece::white_rook][move.destination_square - 2];
                        eg_PST_evaluation -= eg_white_PSTs[Piece::white_rook][move.destination_square - 2];

                        mg_PST_evaluation += mg_white_PSTs[Piece::white_rook][move.destination_square + 1];
                        eg_PST_evaluation += eg_white_PSTs[Piece::white_rook][move.destination_square + 1];

                        current_zobrist_position ^= zobrist_piece_keys[Zobrist::white][Piece::white_rook][move.destination_square - 2];
                        current_zobrist_position ^= zobrist_piece_keys[Zobrist::white][Piece::white_rook][move.destination_square + 1];
                    }
                    else if(piece == Piece::black_king){
                        board[move.destination_square + 1] = Piece::black_rook;
                        board[move.destination_square - 2] = Piece::empty;

                        mg_PST_evaluation += mg_black_PSTs[Piece::white_rook][move.destination_square - 2];
                        eg_PST_evaluation += eg_black_PSTs[Piece::white_rook][move.destination_square - 2];

                        mg_PST_evaluation -= mg_black_PSTs[Piece::white_rook][move.destination_square + 1];
                        eg_PST_evaluation -= eg_black_PSTs[Piece::white_rook][move.destination_square + 1];

                        current_zobrist_position ^= zobrist_piece_keys[Zobrist::black][Piece::white_rook][move.destination_square - 2];
                        current_zobrist_position ^= zobrist_piece_keys[Zobrist::black][Piece::white_rook][move.destination_square + 1];
                    }
                }
            }

            // // update en_passant_square
            // Remove the old en passant from the Zobrist position if there was one
            if (en_passant_square != -1)
            {
                current_zobrist_position ^= zobrist_en_passant_keys[(en_passant_square % BoardPart::columns) - 2];
            }

            en_passant_square = -1;
            if((piece == Piece::white_pawn || piece == Piece::black_pawn) && (move.flag == MoveFlag::double_push)){
                en_passant_square = (move.start_square + move.destination_square) / 2;
                current_zobrist_position ^= zobrist_en_passant_keys[(en_passant_square % BoardPart::columns) - 2]; // -2 because the array starts at 0 but the first column is 2
            }

            //update castling
            //king moves
            if(piece == Piece::white_king){
                if (white_kingside_castle == true)
                {
                    current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::white_kingside_castle];
                }

                if (white_queenside_castle == true)
                {
                    current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::white_queenside_castle];
                }

                white_kingside_castle = false;
                white_queenside_castle = false;

                // Also update the king position
                white_king_position = move.destination_square;
            }
            else if(piece == Piece::black_king){
                if (black_kingside_castle == true)
                {
                    current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::black_kingside_castle];
                }

                if (black_queenside_castle == true)
                {
                    current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::black_queenside_castle];
                }

                black_kingside_castle = false;
                black_queenside_castle = false;
                black_king_position = move.destination_square;
            }
            //rook moved from original square
            if(piece == Piece::white_rook){
                if(move.start_square == white_kingside_rook_starting_square){
                    if (white_kingside_castle == true)
                    {
                        current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::white_kingside_castle];
                    }

                    white_kingside_castle = false;
                }
                else if(move.start_square == white_queenside_rook_starting_square){
                    if (white_queenside_castle == true)
                    {
                        current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::white_queenside_castle];
                    }

                    white_queenside_castle = false;
                }
            }
            else if(piece == Piece::black_rook){
                if(move.start_square == black_kingside_rook_starting_square){
                    if (black_kingside_castle == true)
                    {
                        current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::black_kingside_castle];
                    }

                    black_kingside_castle = false;
                }
                else if(move.start_square == black_queenside_rook_starting_square){
                    if (black_queenside_castle == true)
                    {
                        current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::black_queenside_castle];
                    }

                    black_queenside_castle = false;
                }
            }
            //rook was captured
            if(captured_piece == Piece::white_rook){
                if(move.destination_square == white_kingside_rook_starting_square){
                    if (white_kingside_castle == true)
                    {
                        current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::white_kingside_castle];
                    }

                    white_kingside_castle = false;
                }
                else if(move.destination_square == white_queenside_rook_starting_square){
                    if (white_queenside_castle == true)
                    {
                        current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::white_queenside_castle];
                    }

                    white_queenside_castle = false;
                }
            }
            else if(captured_piece == Piece::black_rook){
                if(move.destination_square == black_kingside_rook_starting_square){
                    if (black_kingside_castle == true)
                    {
                        current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::black_kingside_castle];
                    }

                    black_kingside_castle = false;
                }
                else if(move.destination_square == black_queenside_rook_starting_square){
                    if (black_queenside_castle == true)
                    {
                        current_zobrist_position ^= zobrist_castle_right_keys[Zobrist::black_queenside_castle];
                    }

                    black_queenside_castle = false;
                }
            }

            current_zobrist_position ^= zobrist_white_to_move;
            position_history.push_back(current_zobrist_position);

            return history;
        }

        void undo_move(const Move& move, const MoveHistory& history){
            int piece = board[move.destination_square];
            int captured_piece = history.captured_piece;
            board[move.start_square] = piece;
            board[move.destination_square] = captured_piece;

            //promotion
            if(move.promotion_piece != 0){
                board[move.start_square] = Piece::white_pawn * history.colour;
            }

            // en passant
            // TODO: en passant
            if(move.flag == MoveFlag::en_passant){
                if(piece == Piece::white_pawn){
                    board[move.destination_square + BoardPart::columns] = Piece::black_pawn;
                }
                else if(piece == Piece::black_pawn){
                    board[move.destination_square - BoardPart::columns] = Piece::white_pawn;
                }
            }

            // //castling
            if(move.flag == MoveFlag::castle){
                //king already moved

                //kingside(rightside)
                if(move.destination_square > move.start_square){
                    if(piece == Piece::white_king){
                        board[move.destination_square - 1] = Piece::empty;
                        board[move.destination_square + 1] = Piece::white_rook;
                    }
                    else if(piece == Piece::black_king){
                        board[move.destination_square - 1] = Piece::empty;
                        board[move.destination_square + 1] = Piece::black_rook;
                    }
                }
                else if(move.destination_square < move.start_square){ //queenside(leftside)
                    if(piece == Piece::white_king){
                        board[move.destination_square + 1] = Piece::empty;
                        board[move.destination_square - 2] = Piece::white_rook;
                    }
                    else if(piece == Piece::black_king){
                        board[move.destination_square + 1] = Piece::empty;
                        board[move.destination_square - 2] = Piece::black_rook;
                    }
                }
            }

            // update en_passant_square
            en_passant_square = history.en_passant_square;

            //update castling
            //king moves
            if(piece == Piece::white_king){
                white_kingside_castle = history.white_kingside_castle;
                white_queenside_castle = history.white_queenside_castle;

                // Also update the king position
                white_king_position = move.start_square;
            }
            else if(piece == Piece::black_king){
                black_kingside_castle = history.black_kingside_castle;
                black_queenside_castle = history.black_queenside_castle;
                black_king_position = move.start_square;
            }
            //rook moved from original square
            if(piece == Piece::white_rook){
                if(move.start_square == white_kingside_rook_starting_square){
                    white_kingside_castle = history.white_kingside_castle;
                }
                else if(move.start_square == white_queenside_rook_starting_square){
                    white_queenside_castle = history.white_queenside_castle;
                }
            }
            else if(piece == Piece::black_rook){
                if(move.start_square == black_kingside_rook_starting_square){
                    black_kingside_castle = history.black_kingside_castle;
                }
                else if(move.start_square == black_queenside_rook_starting_square){
                    black_queenside_castle = history.black_queenside_castle;
                }
            }

            //rook was captured
            if(captured_piece == Piece::white_rook){
                if(move.destination_square == white_kingside_rook_starting_square){
                    white_kingside_castle = history.white_kingside_castle;
                }
                else if(move.destination_square == white_queenside_rook_starting_square){
                    white_queenside_castle = history.white_queenside_castle;
                }
            }
            else if(captured_piece == Piece::black_rook){
                if(move.destination_square == black_kingside_rook_starting_square){
                    black_kingside_castle = history.black_kingside_castle;
                }
                else if(move.destination_square == black_queenside_rook_starting_square){
                    black_queenside_castle = history.black_queenside_castle;
                }
            }

            // Restore evaluation variables
            current_piece_material = history.current_piece_material;
            mg_PST_evaluation = history.mg_PST_evaluation;
            eg_PST_evaluation = history.eg_PST_evaluation;
            white_material = history.white_material;
            black_material = history.black_material;
            current_zobrist_position = history.current_zobrist_position;

            // Remove the latest move added by make_move()
            if (!position_history.empty())
            {
                position_history.pop_back();
            }
        }

        unsigned long long perft(int depth, int colour)
        {
            unsigned long long nodes{};

            if (depth == 0)
            {
                return 1ULL;
            }

            MovesInfo moves{ pseudo_legal_move_gen(colour) };
            for (int i{}; i < moves.count; i++)
            {
                Move& move{ moves.moves[i] };
                MoveHistory history{ make_move(move) };
                int target_king_position = (colour == Colour::white) ? white_king_position : black_king_position;

                if (move.flag == MoveFlag::castle || !is_square_attacked(target_king_position, colour)) // Castle moves are already entirely checked if they're legal in the move generation
                {
                    nodes += perft(depth - 1, -colour);
                }

                undo_move(move, history);
            }

            return nodes;
        }
    };

    class Eval
    {
        private:
        public:
        Board& board_class;

        Eval(Board& board)
            : board_class(board)
        {

        }

            // for (int piece : board_class.board)
            // {
            //     int piece_type{ std::abs(piece) };
            //     if (piece_type == Piece::white_pawn || piece_type == Piece::white_king || piece == Piece::empty || piece == BoardPart::padding) continue;
            //     current_piece_material += piece_values[piece_type];

            //     if (current_piece_material > start_piece_material)
            //     {
            //         current_piece_material = start_piece_material;
            //         break;
            //     }
            // }

            // for (int i{}; i < board_class.board.size(); i++)
            // {
            //     int piece{ board_class.board[i] };
            //     if (piece == Piece::empty || piece == BoardPart::padding) continue;
            //     int piece_type{ std::abs(piece) };


            //     if (piece > 0)
            //     {
            //         evaluation += piece_values[piece_type];
            //         PST_evaluation += (current_piece_material * (mg_white_PSTs[piece_type][i] - eg_white_PSTs[piece_type][i]) + start_piece_material * eg_white_PSTs[piece_type][i]);
            //     }
            //     else
            //     {
            //         evaluation -= piece_values[piece_type];
            //         PST_evaluation -= (current_piece_material * (mg_black_PSTs[piece_type][i] - eg_black_PSTs[piece_type][i]) + start_piece_material * eg_black_PSTs[piece_type][i]);
            //     }
            // }

        int evaluate(int colour)
        {
            int evaluation{};
            int local_current_piece_material{ board_class.current_piece_material };

            evaluation += board_class.white_material - board_class.black_material;
            if (local_current_piece_material > board_class.start_piece_material) local_current_piece_material = board_class.start_piece_material; // Clamp the value so the PST values don't get too high
            evaluation += ((local_current_piece_material * (board_class.mg_PST_evaluation - board_class.eg_PST_evaluation)) / board_class.start_piece_material) + board_class.eg_PST_evaluation;

            if (colour == Colour::white)
            {
                return evaluation;
            }
            else
            {
                return -evaluation;
            }
        }
    };

    struct BestMove
    {
        Move move;
        int eval;
    };

    enum class NodeType
    {
        exact = 1,
        lower_bound = 2,
        upper_bound = 3,
    };

    struct TTEntry
    {
        uint64_t zobrist_key;
        int depth;
        int best_score;
        // int age; for now we can just do always replace if the depth is higher
        NodeType flag;
        Move best_move;
    };

    class Search
    {
        private:
        std::vector<TTEntry> transposition_table{};

        public:
        Board& board_class;
        Eval& eval_class;

        Search(Board& board, Eval& eval, uint64_t TT_size)
            : board_class(board), eval_class(eval)
        {
            transposition_table.resize(TT_size);
        }

        void order_moves(MovesInfo& moves, Move& hash_move)
        {
            // MVV LVA
            for (int i{}; i < moves.count; i++)
            {
                Move& move{ moves.moves[i] };
                if (move.start_square == hash_move.start_square && move.destination_square == hash_move.destination_square)
                {
                    move.score = 1000000;
                }
                else if (move.flag == MoveFlag::capture)
                {
                    int victim{ std::abs(board_class.board[move.destination_square]) };
                    int attacker{ std::abs(board_class.board[move.start_square]) };
                    move.score = 10 * victim - attacker;
                }
            }

            std::sort(moves.moves.begin(), moves.moves.begin() + moves.count, [](const Move& a, const Move& b)
                {
                    return a.score > b.score;
                }
            );
        }

        int quiescence(int alpha, int beta, int colour)
        {
            Move hash_move{};

            int best_value{ eval_class.evaluate(colour) };
            if (best_value >= beta)
            {
                return best_value;
            }

            if (best_value > alpha)
            {
                alpha = best_value;
            }

            MovesInfo moves{ board_class.pseudo_legal_move_gen(colour) };
            order_moves(moves, hash_move);
            for (int i{}; i < moves.count; i++)
            {
                Move& move{ moves.moves[i] };
                if (move.flag != MoveFlag::capture && move.flag != MoveFlag::promotion && move.flag != MoveFlag::promotion_capture && move.flag != MoveFlag::en_passant) continue; // We only want to look at these types of moves for now
                MoveHistory history{ board_class.make_move(move) };

                int target_king_position = (colour == Colour::white) ? board_class.white_king_position : board_class.black_king_position;
                if (board_class.is_square_attacked(target_king_position, colour)) // Skip illegal moves
                {
                    board_class.undo_move(move, history);
                    continue;
                }

                int score{ -quiescence(-beta, -alpha, -colour) };
                board_class.undo_move(move, history);

                if (score >= beta)
                {
                    return score;
                }

                if (score > best_value)
                {
                    best_value = score;
                }

                if (score > alpha)
                {
                    alpha = score;
                }
            }
            return best_value;
        }

        int negamax(int alpha, int beta, int depth, int colour)
        {
            int original_alpha{ alpha };
            size_t TT_index{ board_class.current_zobrist_position % transposition_table.size() };
            TTEntry& TT_entry{ transposition_table[TT_index] };
            Move hash_move{};

            if (TT_entry.zobrist_key == board_class.current_zobrist_position)
            {
                hash_move = TT_entry.best_move;
                if (TT_entry.depth >= depth)
                {
                    if (TT_entry.flag == NodeType::exact)
                    {
                        return TT_entry.best_score;
                    }
                    else if (TT_entry.flag == NodeType::lower_bound && TT_entry.best_score >= beta)
                    {
                        return TT_entry.best_score;
                    }
                    else if (TT_entry.flag == NodeType::upper_bound && TT_entry.best_score <= alpha)
                    {
                        return TT_entry.best_score;
                    }
                }
            }

            if (depth == 0)
            {
                return quiescence(alpha, beta, colour);
            }

            int best_score{ -1000000000 };
            Move best_move{};
            MovesInfo moves{ board_class.pseudo_legal_move_gen(colour) };
            order_moves(moves, hash_move);
            for (int i{}; i < moves.count; i++)
            {
                Move& move{ moves.moves[i] };
                MoveHistory history{board_class.make_move(move) };

                int target_king_position = (colour == Colour::white) ? board_class.white_king_position : board_class.black_king_position;
                if (board_class.is_square_attacked(target_king_position, colour)) // Skip illegal moves
                {
                    board_class.undo_move(move, history);
                    continue;
                }

                // Threefold repetition rule
                bool is_threefold{ false };
                int repetitions{};

                for (int j{ static_cast<int>(board_class.position_history.size()) - 1}; j >= 0; j--)
                {
                    uint64_t position{ board_class.position_history[j] };
                    if (position == board_class.current_zobrist_position)
                    {
                        repetitions++;
                        if (repetitions == 2)
                        {
                            board_class.undo_move(move, history);
                            return 0;
                        }
                    }
                }

                int score{ -negamax(-beta, -alpha, depth - 1, -colour) };
                board_class.undo_move(move, history);

                if (score > best_score)
                {
                    best_score = score;
                    if (score > alpha)
                    {
                        best_move = move;
                        alpha = score;
                    }
                }

                if (score >= beta)
                {
                    if (TT_entry.zobrist_key == 0 || TT_entry.depth <= depth)
                    {
                        TT_entry.zobrist_key = board_class.current_zobrist_position;
                        TT_entry.best_score = best_score;
                        TT_entry.depth = depth;
                        TT_entry.flag = NodeType::lower_bound;
                        TT_entry.best_move = move;
                    }
                    return best_score;
                }
            }

            if (TT_entry.zobrist_key == 0 || TT_entry.depth <= depth)
            {
                if (best_score <= original_alpha)
                {
                    TT_entry.flag = NodeType::upper_bound;
                    TT_entry.best_move = Move{};
                }
                else
                {
                    TT_entry.flag = NodeType::exact;
                    TT_entry.best_move = best_move;
                }

                TT_entry.zobrist_key = board_class.current_zobrist_position;
                TT_entry.best_score = best_score;
                TT_entry.depth = depth;
            }

            return best_score;
        }

        BestMove get_best_move(int depth, int colour)
        {
            Move previous_best_move{};
            BestMove best_move{};
            best_move.eval = -2000000000;

            for (int current_depth{ 1 }; current_depth <= depth; current_depth++)
            {   
                int alpha{ -1000000000 };
                static constexpr int beta{ 1000000000 };

                int current_depth_best_score{ -2000000000 };
                Move current_depth_best_move{};

                MovesInfo moves{ board_class.pseudo_legal_move_gen(colour) };

                order_moves(moves, previous_best_move);
                for (int i{}; i < moves.count; i++)
                {
                    Move& move{ moves.moves[i] };
                    MoveHistory history{board_class.make_move(move) };

                    int target_king_position = (colour == Colour::white) ? board_class.white_king_position : board_class.black_king_position;
                    if (board_class.is_square_attacked(target_king_position, colour)) // Skip illegal moves
                    {
                        board_class.undo_move(move, history);
                        continue;
                    }

                    int score{ -negamax(-beta, -alpha, current_depth - 1, -colour) };
                    board_class.undo_move(move, history);

                    if (score > current_depth_best_score)
                    {
                        current_depth_best_score = score;
                        current_depth_best_move = move;

                        if (score > alpha)
                        {
                            alpha = score;
                        }
                    }
                }

                previous_best_move = current_depth_best_move;
                best_move.move = current_depth_best_move;
                best_move.eval = current_depth_best_score;
            }
            return best_move;
        }
    };
}

#endif