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

    class Eval
    {
        private:
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

        int evaluate()
        {
            int evaluation{};
            for (int piece : board_class.board)
            {
                int colour = (piece > 0) ? Colour::white : Colour::black;
                switch (std::abs(piece))
                {
                    case Piece::white_pawn:
                        evaluation += piece_values[Piece::white_pawn] * colour;
                        break;
                    case Piece::white_knight:
                        evaluation += piece_values[Piece::white_knight] * colour;
                        break;
                    case Piece::white_bishop:
                        evaluation += piece_values[Piece::white_bishop] * colour;
                        break;
                    case Piece::white_rook:
                        evaluation += piece_values[Piece::white_rook] * colour;
                        break;
                    case Piece::white_queen:
                        evaluation += piece_values[Piece::white_queen] * colour;
                        break;
                    case Piece::white_king:
                        evaluation += piece_values[Piece::white_king] * colour;
                        break;
                    default:
                        continue;   
                }
            }

            return evaluation;
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
            int best_value{ eval_class.evaluate() * colour };
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
                if (move.flag != MoveFlag::capture && move.flag != MoveFlag::promotion && move.flag != MoveFlag::promotion_capture) continue; // We only want to look at these types of moves for now
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