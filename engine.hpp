#include <array>
#include <iostream>
#include <unordered_map>
#include <cctype>
#include <algorithm>

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
    };

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

            int piece = board[move.start_square];
            int captured_piece = board[move.destination_square]; //helpful for updating castling conditions
            board[move.destination_square] = piece;
            board[move.start_square] = Piece::empty;

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
            }

            // en passant
            if(move.flag == MoveFlag::en_passant){
                if(piece == Piece::white_pawn){
                    board[move.destination_square + BoardPart::columns] = Piece::empty;
                }
                else if(piece == Piece::black_pawn){
                    board[move.destination_square - BoardPart::columns] = Piece::empty;
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
                    }
                    else if(piece == Piece::black_king){
                        board[move.destination_square - 1] = Piece::black_rook;
                        board[move.destination_square + 1] = Piece::empty;
                    }
                }
                else if(move.destination_square < move.start_square){ //queenside(leftside)
                    if(piece == Piece::white_king){
                        board[move.destination_square + 1] = Piece::white_rook;
                        board[move.destination_square - 2] = Piece::empty;
                    }
                    else if(piece == Piece::black_king){
                        board[move.destination_square + 1] = Piece::black_rook;
                        board[move.destination_square - 2] = Piece::empty;
                    }
                }
            }

            // // update en_passant_square
            en_passant_square = -1;
            if((piece == Piece::white_pawn || piece == Piece::black_pawn) && (move.flag == MoveFlag::double_push)){
                en_passant_square = (move.start_square + move.destination_square) / 2;
            }

            //update castling
            //king moves
            if(piece == Piece::white_king){
                white_kingside_castle = false;
                white_queenside_castle = false;

                // Also update the king position
                white_king_position = move.destination_square;
            }
            else if(piece == Piece::black_king){
                black_kingside_castle = false;
                black_queenside_castle = false;
                black_king_position = move.destination_square;
            }
            //rook moved from original square
            if(piece == Piece::white_rook){
                if(move.start_square == white_kingside_rook_starting_square){
                    white_kingside_castle = false;
                }
                else if(move.start_square == white_queenside_rook_starting_square){
                    white_queenside_castle = false;
                }
            }
            else if(piece == Piece::black_rook){
                if(move.start_square == black_kingside_rook_starting_square){
                    black_kingside_castle = false;
                }
                else if(move.start_square == black_queenside_rook_starting_square){
                    black_queenside_castle = false;
                }
            }
            //rook was captured
            if(captured_piece == Piece::white_rook){
                if(move.destination_square == white_kingside_rook_starting_square){
                    white_kingside_castle = false;
                }
                else if(move.destination_square == white_queenside_rook_starting_square){
                    white_queenside_castle = false;
                }
            }
            else if(captured_piece == Piece::black_rook){
                if(move.destination_square == black_kingside_rook_starting_square){
                    black_kingside_castle = false;
                }
                else if(move.destination_square == black_queenside_rook_starting_square){
                    black_queenside_castle = false;
                }
            }

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

    class Eval
    {
        private:
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

        static constexpr std::array<std::array<int, 144>, 7> mg_black_PSTs = generate_black_PSTs(mg_white_PSTs);

        // We can change these later
        static constexpr std::array<int, 7> piece_values
        {
            // padding, P, N, B, R, Q, K
            0, 100, 300, 350, 500, 1000, 10000
        };

        public:
        Board& board_class;

        Eval(Board& board)
            : board_class(board)
        {

        }

        int evaluate(int colour)
        {
            int evaluation{};
            for (int i{}; i < board_class.board.size(); i++)
            {
                int piece{ board_class.board[i] };
                if (piece == Piece::empty || piece == BoardPart::padding) continue;
                int piece_type{ std::abs(piece) };


                if (piece > 0)
                {
                    evaluation += piece_values[piece_type];
                    evaluation += mg_white_PSTs[piece_type][i];
                }
                else
                {
                    evaluation -= piece_values[piece_type];
                    evaluation -= mg_black_PSTs[piece_type][i];
                }
            }

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

    class Search
    {
        private:
        public:
        Board& board_class;
        Eval& eval_class;

        Search(Board& board, Eval& eval)
            : board_class(board), eval_class(eval)
        {

        }

        void order_moves(MovesInfo& moves)
        {
            // MVV LVA
            for (int i{}; i < moves.count; i++)
            {
                Move& move{ moves.moves[i] };
                if (move.flag == MoveFlag::capture)
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
            order_moves(moves);
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
            if (depth == 0)
            {
                return quiescence(alpha, beta, colour);
            }

            int best_score{ -1000000000 };
            MovesInfo moves{ board_class.pseudo_legal_move_gen(colour) };
            order_moves(moves);
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

                int score{ -negamax(-beta, -alpha, depth - 1, -colour) };
                board_class.undo_move(move, history);

                if (score > best_score)
                {
                    best_score = score;
                    if (score > alpha)
                    {
                        alpha = score;
                    }
                }

                if (score >= beta)
                {
                    return best_score;
                }
            }
            return best_score;
        }

        BestMove get_best_move(int depth, int colour)
        {
            int alpha{ -1000000000 };
            static constexpr int beta{ 1000000000 };

            BestMove best_move{};
            best_move.eval = alpha;
            
            MovesInfo moves{ board_class.pseudo_legal_move_gen(colour) };
            order_moves(moves);
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

                int score{ -negamax(-beta, -alpha, depth - 1, -colour) };
                board_class.undo_move(move, history);

                if (score > best_move.eval)
                {
                    best_move.eval = score;
                    best_move.move = move;

                    if (score > alpha)
                    {
                        alpha = score;
                    }
                }
            }
            return best_move;
        }

    };
}