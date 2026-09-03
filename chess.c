#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#define MAX_MOVES 256 // To preallocate memory for storing move possibilities. Should be increased for non-standard chess positions

typedef enum
{
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
} Coordinate;


typedef enum
{
    MOVE_NORMAL,
    MOVE_EN_PASSANT,
    MOVE_DOUBLE_PUSH,
    MOVE_CASTLE_K,
    MOVE_CASTLE_Q,
    MOVE_PROMOTION_Q,
    MOVE_PROMOTION_R,
    MOVE_PROMOTION_B,
    MOVE_PROMOTION_N,
    AMOUNT_OF_MOVE_TYPES
} MoveType;

// Move struct
typedef struct
{
    uint8_t from; //    --- 1 byte
    uint8_t to; //      --- 1 byte
    uint8_t special; // --- 1 byte
} Move;

typedef enum
{
    WHITE_TO_MOVE_FLAG      = 1,
    WHITE_CAN_CASTLE_K_FLAG = 2,
    WHITE_CAN_CASTLE_Q_FLAG = 4,
    BLACK_CAN_CASTLE_K_FLAG = 8,
    BLACK_CAN_CASTLE_Q_FLAG = 16
} MetadataFlag;

// BoardState struct
typedef struct
{
    uint64_t board[4]; // 4 bitboards storing 4 bits for every cell  --- 32 bytes
    // The first bit represents color (0: black, 1: white)
    // - 0000: empty (0)
    // - 0001: black king (1)   - 1001: white king (9)
    // - 0010: black queen (2)  - 1010: white queen (10)
    // - 0011: black rook (3)   - 1011: white rook (11)
    // - 0100: black bishop (4) - 1100: white bishop (12)
    // - 0101: black knight (5) - 1101: white knight (13)
    // - 0110: black pawn (6)   - 1110: white pawn (14)
    // A1 (0) is at the least significant bit
    // H8 (63) is at the most significant bit
    // The 4 bitboards are in order, bb ID 0 has the MSB, and bb ID 3 has the LSB

    uint8_t enPassantPos; // default 0: no en passant.                --- 1 byte

    uint8_t metadata; // Stores castling rights and player to move.   --- 1 byte
    
} BoardState; // 34 bytes total

typedef enum 
{
    EMPTY_RELATION,
    FRIEND,
    ENEMY
} Relation ;

typedef enum
{
    EMPTY_PIECE,
    BLACK_KING,
    BLACK_QUEEN,
    BLACK_ROOK,
    BLACK_BISHOP,
    BLACK_KNIGHT,
    BLACK_PAWN,
    WHITE_KING = 9,
    WHITE_QUEEN = 10,
    WHITE_ROOK = 11,
    WHITE_BISHOP = 12,
    WHITE_KNIGHT = 13,
    WHITE_PAWN = 14
} PieceType;

static const char PIECE_CHARS[] = 
{
    [0] = '.', // empty
    [1] = 'k', // black
    [2] = 'q',
    [3] = 'r',
    [4] = 'b',
    [5] = 'n',
    [6] = 'p',
    [9] = 'K', // white
    [10] = 'Q',
    [11] = 'R',
    [12] = 'B',
    [13] = 'N',
    [14] = 'P'
};

static const char COORDINATES[64][3] =
{
    "A1", "B1", "C1", "D1", "E1", "F1", "G1", "H1",
    "A2", "B2", "C2", "D2", "E2", "F2", "G2", "H2",
    "A3", "B3", "C3", "D3", "E3", "F3", "G3", "H3",
    "A4", "B4", "C4", "D4", "E4", "F4", "G4", "H4",
    "A5", "B5", "C5", "D5", "E5", "F5", "G5", "H5",
    "A6", "B6", "C6", "D6", "E6", "F6", "G6", "H6",
    "A7", "B7", "C7", "D7", "E7", "F7", "G7", "H7",
    "A8", "B8", "C8", "D8", "E8", "F8", "G8", "H8"
};

static const int8_t KNIGHT_OFFSETS[8][2] =
{
    {-2, -1},
    {-2, 1},
    {-1, -2},
    {-1, 2},
    {1, -2},
    {1, 2},
    {2, -1},
    {2, 1}
};
static const int8_t ORTHOGONAL_OFFSETS[4][2] =
{
    {-1, 0},
    {1, 0},
    {0, -1},
    {0, 1}
};
static const int8_t DIAGONAL_OFFSETS[4][2] =
{
    {-1, -1},
    {-1, 1},
    {1, -1},
    {1, 1}
};


static const char* const MOVE_TYPE_NAMES[AMOUNT_OF_MOVE_TYPES] =
{
    [MOVE_NORMAL] = "",
    [MOVE_EN_PASSANT] = "en passant",
    [MOVE_DOUBLE_PUSH] = "double push",
    [MOVE_CASTLE_K] = "kingside castle",
    [MOVE_CASTLE_Q] = "queenside castle",
    [MOVE_PROMOTION_Q] = "promotion to queen",
    [MOVE_PROMOTION_R] = "promotion to rook",
    [MOVE_PROMOTION_B] = "promotion to bishop",
    [MOVE_PROMOTION_N] = "promotion to knight"

};

static const BoardState EMPTY_BOARD =
{
    {
        0ULL,
        0ULL,
        0ULL,
        0ULL
    },
    0,
    1
};
static const BoardState START_BOARD =
{
    {             
        65535ULL,
        7421650710929932134ULL,
        9943666502257409929ULL,
        15204152342002794707ULL
        // K    Q   R     B    N    P       k                     q                    r                     b                     n                     p
        // 16 + 8 + 129 + 36 + 66 + 65280 + 0                   + 0                  + 0                   + 0                   + 0                   + 0
        // 0  + 0 + 0   + 36 + 66 + 65280 + 0                   + 0                  + 0                   + 2594073385365405696 + 4755801206503243776 + 71776119061217280
        // 0  + 8 + 129 + 0  + 0  + 65280 + 0                   + 576460752303423488 + 9295429630892703744 + 0                   + 0                   + 71776119061217280
        // 16 + 0 + 129 + 0  + 66 + 0     + 1152921504606846976 + 0                  + 9295429630892703744 + 0                   + 4755801206503243776 + 0
    },
    0,
    WHITE_TO_MOVE_FLAG | WHITE_CAN_CASTLE_K_FLAG | WHITE_CAN_CASTLE_Q_FLAG | BLACK_CAN_CASTLE_K_FLAG | BLACK_CAN_CASTLE_Q_FLAG
};

// ------------------------------------------- DECLARATIONS ----------------------------------------

static inline int bitscanForward(uint64_t bitboard);
uint8_t getPiece(BoardState *state, uint8_t pos);
void setPiece(BoardState *state, uint8_t pos, uint8_t piece); // Pos: [0-63], piece: {0:empty, [1-6]:white [9-14]:black}
void makeMove(BoardState *state, Move *move);
void printBoard(BoardState *state);
void printMoves(BoardState *state, Move *moves, int move_count);
Move* getAllValidBoardMoves(BoardState *state, int *move_count); // Generates all valid moves, updates a move counter variable in place and returns a pointer to the new Move array.
void getAllPseudoValidPieceMoves(BoardState *state, uint8_t pos, Move *move_list, int *count); // Modifies an array of moves in place with the valid moves of a piece. Updates the move count too
uint8_t getOffsetPosition(uint8_t pos, int8_t dx, int8_t dy); // Returns a position after some offset. If out of bounds, returns the original position
Relation getRelationToSelf(uint8_t self, uint8_t other); // FRIEND, ENEMY or EMPTY_RELATION
void analyzeKingSafety(BoardState *state, uint64_t *out_pinned_pieces_mask, uint64_t *out_attackers_mask, uint64_t *out_attacked_squares_mask); // Calculates pins, checks and unsafe squares
void getAllAttackedSquares(BoardState *state, uint64_t *out_attacked_squares_mask, uint64_t enemy_pieces_mask); // Calculates the squares the enemy color is attacking (Including their own pieces)

int main()
{
    BoardState bstate = START_BOARD; // Default board
    // bstate.metadata &= ~WHITE_TO_MOVE_FLAG;
    //setPiece(&bstate, H4, WHITE_ROOK);
    setPiece(&bstate, D5, WHITE_QUEEN);
    //bstate.enPassantPos = 32;
    printBoard(&bstate);



    int moves_count = 0;
    Move *moves = getAllValidBoardMoves(&bstate, &moves_count);
    printMoves(&bstate, moves, moves_count);

    makeMove(&bstate, &(moves[0]));
    printBoard(&bstate);
    moves_count = 0;
    moves = getAllValidBoardMoves(&bstate, &moves_count);
    printMoves(&bstate, moves, moves_count);
    return 0;
}


static inline int bitscanForward(uint64_t bitboard) // Counts trailing zeroes
{
    assert(bitboard != 0);
    return __builtin_ctzll(bitboard);
}

uint8_t getPiece(BoardState *state, uint8_t pos)
{
    uint64_t *board = state->board;
    uint8_t piece = 0;
    for (int i = 0; i<4; i++)
    {   
        piece |= ( (uint8_t)((board[3-i] >> pos) & 1ULL ) ) << i;
        //printf("piece %d", ( (uint8_t)((board[3-i] >> pos) & 1ULL ) ) << i);
    }
    return piece;
}

void setPiece(BoardState *state, uint8_t pos, uint8_t piece)
{
    uint64_t bit;
    for (int i = 0; i<4; i++) // For each bitboard
    {   
        bit = (piece >> i) & 1ULL; // gets the new bit to set for each bitboard
        state->board[3-i] &= (~(1ULL << pos)); // sets the bit to 0
        state->board[3-i] |= ( bit << pos ); // ORs the bit with the new bit
    }
    
}

void makeMove(BoardState *state, Move *move)
{
    if (move->from == move->to)
    {
        printf("Tried to move to itself ");
        printMoves(state, move, 1);
        return;
    }
    MoveType move_type = (move->special);
    state->enPassantPos = 0; // Defaults en passant position to 0
    switch (move_type)
    {
    case MOVE_NORMAL: // Normal move
        setPiece(state, move->to, getPiece(state, move->from));
        setPiece(state, move->from, EMPTY_PIECE);
        break;
    case MOVE_EN_PASSANT: // En passant
        setPiece(state, move->to, getPiece(state, move->from));
        setPiece(state, move->from, EMPTY_PIECE);
        // int y = (move->from) - (move->from)%8; // file
        // int x = (move->to)%8; // rank
        setPiece(state, (move->from) - (move->from)%8 + (move->to)%8, EMPTY_PIECE);
    case MOVE_DOUBLE_PUSH: // Double push
        setPiece(state, move->to, getPiece(state, move->from));
        setPiece(state, move->from, EMPTY_PIECE);
        state->enPassantPos = ((move->to)+(move->from))>>1; // Gets the square in between by adding the positions and dividing by 2
        break;
    case MOVE_CASTLE_K:
        break;
    case MOVE_CASTLE_Q:
        break;
    case MOVE_PROMOTION_Q:
        break;
    case MOVE_PROMOTION_R:
        break;
    case MOVE_PROMOTION_B:
        break;
    case MOVE_PROMOTION_N:
        break;
    default:
        break;
    }
}

void printBoard(BoardState *state)
{
    printf("\n");
    if (state->metadata & WHITE_TO_MOVE_FLAG)
    {
        printf("White to move\n");
    }
    else
    {
        printf("Black to move\n");
    }
    printf(" ________________\n");
    for (int i = 7; i>=0; i--)
    {   
        for (int j = 0; j<8; j++)
        {
            printf(" %c", PIECE_CHARS[getPiece(state, i*8+j)]);
        }
        printf("|%d\n", i+1);
    }
    //printf("-|-|-|-|-|-|-|-\n");
    printf(" a|b|c|d|e|f|g|h|\n\n");
}
void printMoves(BoardState *state, Move *moves, int move_count)
{
    if (move_count == 0)
    {
        printf("\nNo valid moves\n");
        return;
    }
    for (int i = 0; i<move_count; i++)
    {   
        Move *move = &moves[i];
        PieceType piece_type = getPiece(state, move->from);
        printf("%d- %c From %s to %s, %s\n",i, PIECE_CHARS[piece_type], COORDINATES[move->from], COORDINATES[move->to], MOVE_TYPE_NAMES[move->special]);
    }
}

uint8_t getOffsetPosition(uint8_t pos, int8_t dx, int8_t dy)
{
    int8_t x = (pos & 7) + dx; // Equivalent to doing mod 8
    int8_t y = (pos >> 3) + dy; // Equivalent to whole division by 8
    if (x < 0 || y < 0 || x > 7 || y > 7)
    {
        return pos; // Returns the original position if the offset goes out of bounds
    }
    return (y<<3) + x; // Returns the position after the offset
}

Relation getRelationToSelf(uint8_t self, uint8_t other)
{
    if (other == 0)
    {
        return EMPTY_RELATION; 
    }
    if ((self >> 3) != (other >> 3))
    {
        return ENEMY;
    }
    return FRIEND;
}

void getAllPseudoValidPieceMoves(BoardState *state, uint8_t pos, Move *move_list, int *count)
{
    uint8_t self_piece = getPiece(state, pos);
    uint8_t target_pos;
    uint8_t other_piece;
    uint8_t relation;
    switch (self_piece)
    {
        case 0: // empty
            break;
        case 1: // black king
        case 9: //white king
            // Cycles through the 8 positions around the king
            for (int i = 0; i<4; i++) // 4 orthogonal positions
            {
                target_pos = getOffsetPosition(pos, ORTHOGONAL_OFFSETS[i][0], ORTHOGONAL_OFFSETS[i][1]);
                relation = getRelationToSelf(self_piece, getPiece(state, target_pos));
                if (relation != FRIEND)
                {
                    // CHECK IF MOVE IS CHECK-SAFE
                    move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                    (*count)++;
                }
            }
            for (int i = 0; i<4; i++) // 4 diagonal positions
            {
                target_pos = getOffsetPosition(pos, DIAGONAL_OFFSETS[i][0], DIAGONAL_OFFSETS[i][1]);
                relation = getRelationToSelf(self_piece, getPiece(state, target_pos));
                if (relation != FRIEND)
                {
                    // CHECK IF MOVE IS CHECK-SAFE
                    move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                    (*count)++;
                }
            }
            break;
        case 2: // black queen
        case 10: // white queen
        {
            uint8_t length;
            for (int i = 0; i<4; i++)
            {
                length = 1;
                target_pos = getOffsetPosition(pos, ORTHOGONAL_OFFSETS[i][0], ORTHOGONAL_OFFSETS[i][1]);
                other_piece = getPiece(state, target_pos);
                while (other_piece == EMPTY_PIECE)
                {
                    move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                    (*count)++;
                    length++;
                    target_pos = getOffsetPosition(pos, ORTHOGONAL_OFFSETS[i][0] * length, ORTHOGONAL_OFFSETS[i][1] * length);
                    other_piece = getPiece(state, target_pos);
                }
                if (getRelationToSelf(self_piece, other_piece) == ENEMY)
                {
                    move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                    (*count)++;
                }
            }
            for (int i = 0; i<4; i++)
            {
                length = 1;
                target_pos = getOffsetPosition(pos, DIAGONAL_OFFSETS[i][0], DIAGONAL_OFFSETS[i][1]);
                other_piece = getPiece(state, target_pos);
                while (other_piece == EMPTY_PIECE)
                {
                    move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                    (*count)++;
                    length++;
                    target_pos = getOffsetPosition(pos, DIAGONAL_OFFSETS[i][0] * length, DIAGONAL_OFFSETS[i][1] * length);
                    other_piece = getPiece(state, target_pos);
                }
                if (getRelationToSelf(self_piece, other_piece) == ENEMY)
                {
                    move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                    (*count)++;
                }
            }
            break;
        }
        case 3: // black rook
        case 4: // black bishop
        case 11: // white rook
        case 12: // white bishop
        {
            int8_t const (*offsets_const)[2];
            if (self_piece == WHITE_ROOK || self_piece == BLACK_ROOK)
            {
                offsets_const = ORTHOGONAL_OFFSETS;
            }
            else
            {
                offsets_const = DIAGONAL_OFFSETS;
            }
            for (int i = 0; i<4; i++)
            {
                uint8_t length = 1;
                target_pos = getOffsetPosition(pos, offsets_const[i][0], offsets_const[i][1]);
                other_piece = getPiece(state, target_pos);
                while (other_piece == EMPTY_PIECE)
                {
                    move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                    (*count)++;
                    length++;
                    target_pos = getOffsetPosition(pos, offsets_const[i][0] * length, offsets_const[i][1] * length);
                    other_piece = getPiece(state, target_pos);
                }
                if (getRelationToSelf(self_piece, other_piece) == ENEMY)
                {
                    move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                    (*count)++;
                }
            }
            break;
        }
        case 5: // black knight
        case 13: // white knight
            for (int i = 0; i<8; i++)
            {
                target_pos = getOffsetPosition(pos, KNIGHT_OFFSETS[i][0], KNIGHT_OFFSETS[i][1]);
                relation = getRelationToSelf(self_piece, getPiece(state, target_pos));
                if (relation != FRIEND)
                {
                    move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                    (*count)++;
                }
            }
            break;
        case 6: // black pawn
        case 14: // white pawn
        {
            uint8_t y_direction;
            uint8_t start_rank;
            bool is_on_start_rank = false;
            // White and black pawns only differ in direction and double push positions
            if (self_piece == WHITE_PAWN)
            {
                y_direction = 1;
                start_rank = 1;
            }
            else
            {
                y_direction = -1;
                start_rank = 6;
            }
            if (pos>>3 == start_rank)
            {
                is_on_start_rank = true; // Checks if the pawn is on the starting rank, to prevent en passant on friendly pieces and double push on non-starting ranks
            }
            
            target_pos = getOffsetPosition(pos, -1, y_direction);
            relation = getRelationToSelf(self_piece, getPiece(state, target_pos));
            if (relation == ENEMY) // Check enemy diagonal up left
            {
                move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                (*count)++;
            }
            else if (!is_on_start_rank && (relation == EMPTY_RELATION) && ((state->enPassantPos) == target_pos)) // Check for en passant diagonal left
            {
                move_list[*count] = (Move){pos, target_pos, MOVE_EN_PASSANT};
                (*count)++;
            }
            
            target_pos = getOffsetPosition(pos, 1, y_direction);
            relation = getRelationToSelf(self_piece, getPiece(state, target_pos));
            if (relation == ENEMY) // Check enemy diagonal right
            {
                move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                (*count)++;
            }
            else if (!is_on_start_rank && (relation == EMPTY_RELATION) && ((state->enPassantPos) == target_pos)) // Check for en passant diagonal right
            {
                move_list[*count] = (Move){pos, target_pos, MOVE_EN_PASSANT};
                (*count)++;
            }

            target_pos = getOffsetPosition(pos, 0, y_direction);
            if (getRelationToSelf(self_piece, getPiece(state, target_pos)) == EMPTY_RELATION) // Check forward 1
            {
                move_list[*count] = (Move){pos, target_pos, MOVE_NORMAL};
                (*count)++;
            }
            else {break;} // Breaks if the up 1 square is occupied

            if (!is_on_start_rank) // Breaks if the pawn is not on the starting rank
            {
                break;
            }
            target_pos = getOffsetPosition(pos, 0, y_direction*2);
            if (getRelationToSelf(self_piece, getPiece(state, target_pos)) == EMPTY_RELATION) // Check forward 2
            {
                move_list[*count] = (Move){pos, target_pos, MOVE_DOUBLE_PUSH};
                (*count)++;
            }
            break;
        }
        default:
            break;
    }
    return;
}

void getAllAttackedSquares(BoardState *state, uint64_t *out_attacked_squares_mask, uint64_t enemy_pieces_mask)
{
    uint8_t piece_pos;
    uint8_t self_piece;
    uint8_t target_pos;
    uint8_t other_piece;
    uint8_t relation;
    while (enemy_pieces_mask != 0) // Cycles through the enemy pieces
    {
        piece_pos = bitscanForward(enemy_pieces_mask);
        self_piece = getPiece(state, piece_pos);
        switch (self_piece) // Adds the attacked squares of a piece depending on its type
        {
            case 0: // empty
                break;
            case 1: // black king
            case 9: //white king
                // Cycles through the 8 positions around the king
                for (int i = 0; i<4; i++) // 4 orthogonal positions
                {
                    target_pos = getOffsetPosition(piece_pos, ORTHOGONAL_OFFSETS[i][0], ORTHOGONAL_OFFSETS[i][1]);
                    if (target_pos != piece_pos)
                    {
                        *out_attacked_squares_mask |= (1ULL << target_pos);
                    }
                }
                for (int i = 0; i<4; i++) // 4 diagonal positions
                {
                    target_pos = getOffsetPosition(piece_pos, DIAGONAL_OFFSETS[i][0], DIAGONAL_OFFSETS[i][1]);
                    if (target_pos != piece_pos)
                    {
                        *out_attacked_squares_mask |= (1ULL << target_pos);
                    }
                }
                break;
            case 2: // black queen
            case 10: // white queen
            {
                uint8_t length;
                for (int i = 0; i<4; i++)
                {
                    length = 1;
                    target_pos = getOffsetPosition(piece_pos, ORTHOGONAL_OFFSETS[i][0], ORTHOGONAL_OFFSETS[i][1]);
                    other_piece = getPiece(state, target_pos);
                    while (other_piece == EMPTY_PIECE)
                    {
                        *out_attacked_squares_mask |= (1ULL << target_pos);
                        length++;
                        target_pos = getOffsetPosition(piece_pos, ORTHOGONAL_OFFSETS[i][0] * length, ORTHOGONAL_OFFSETS[i][1] * length);
                        other_piece = getPiece(state, target_pos);
                    }
                    if (piece_pos != target_pos) // Also adds the end of the ray to the attacked squares (Does out of bounds check before)
                    {
                        *out_attacked_squares_mask |= (1ULL << target_pos);
                    }
                }
                for (int i = 0; i<4; i++)
                {
                    length = 1;
                    target_pos = getOffsetPosition(piece_pos, DIAGONAL_OFFSETS[i][0], DIAGONAL_OFFSETS[i][1]);
                    other_piece = getPiece(state, target_pos);
                    while (other_piece == EMPTY_PIECE)
                    {
                        *out_attacked_squares_mask |= (1ULL << target_pos);
                        length++;
                        target_pos = getOffsetPosition(piece_pos, DIAGONAL_OFFSETS[i][0] * length, DIAGONAL_OFFSETS[i][1] * length);
                        other_piece = getPiece(state, target_pos);
                    }
                    if (piece_pos != target_pos) // Also adds the end of the ray to the attacked squares (Does out of bounds check before)
                    {
                        *out_attacked_squares_mask |= (1ULL << target_pos);
                    }
                }
                break;
            }
            case 3: // black rook
            case 4: // black bishop
            case 11: // white rook
            case 12: // white bishop
            {
                int8_t const (*offsets_const)[2];
                int8_t length;
                if (self_piece == WHITE_ROOK || self_piece == BLACK_ROOK)
                {
                    offsets_const = ORTHOGONAL_OFFSETS;
                }
                else
                {
                    offsets_const = DIAGONAL_OFFSETS;
                }
                for (int i = 0; i<4; i++)
                {
                    length = 1;
                    target_pos = getOffsetPosition(piece_pos, offsets_const[i][0], offsets_const[i][1]);
                    other_piece = getPiece(state, target_pos);
                    while (other_piece == EMPTY_PIECE)
                    {
                        *out_attacked_squares_mask |= (1ULL << target_pos);
                        length++;
                        target_pos = getOffsetPosition(piece_pos, offsets_const[i][0] * length, offsets_const[i][1] * length);
                        other_piece = getPiece(state, target_pos);
                    }
                    if (piece_pos != target_pos) // Also adds the end of the ray to the attacked squares (Does out of bounds check before)
                    {
                        *out_attacked_squares_mask |= (1ULL << target_pos);
                    }
                }
                break;
            }
            case 5: // black knight
            case 13: // white knight
                for (int i = 0; i<8; i++)
                {
                    target_pos = getOffsetPosition(piece_pos, KNIGHT_OFFSETS[i][0], KNIGHT_OFFSETS[i][1]);
                    if (target_pos != piece_pos) // Accounts for out of bounds positions
                    {
                        *out_attacked_squares_mask |= (1ULL << target_pos);
                    }
                }
                break;
            case 6: // black pawn
            case 14: // white pawn
            {
                uint8_t y_direction;
                if (self_piece == WHITE_PAWN)
                {
                    y_direction = 1;
                }
                else
                {
                    y_direction = -1;
                }
                target_pos = getOffsetPosition(piece_pos, -1, y_direction);
                if (target_pos != piece_pos) // Check enemy diagonal up left
                {
                    *out_attacked_squares_mask |= (1ULL << target_pos);
                }
                target_pos = getOffsetPosition(piece_pos, 1, y_direction);
                if (target_pos != piece_pos) // Check enemy diagonal up left
                {
                    *out_attacked_squares_mask |= (1ULL << target_pos);
                }
                break;
            }
            default:
                break;
        }
        return;
        enemy_pieces_mask &= (enemy_pieces_mask-1); // Goes to the next enemy piece
    }
}

void analyzeKingSafety(BoardState *state, uint64_t *out_pinned_pieces_mask, uint64_t *out_attackers_mask, uint64_t *out_attacked_squares_mask)
{
    uint8_t white_to_move = (state->metadata) & WHITE_TO_MOVE_FLAG;
    uint64_t color_mask = 0ULL;
    uint8_t king_pos;
    uint8_t self_piece;
    if (white_to_move)
    {
        color_mask = UINT64_MAX;
        king_pos = bitscanForward(state->board[0] && ~(state->board[1]) && ~(state->board[2]) && state->board[3]); // Gets white king position (1001)
        self_piece = WHITE_KING;
    }
    else
    {
        king_pos = bitscanForward(~(state->board[0]) && ~(state->board[1]) && ~(state->board[2]) && state->board[3]); // Gets black king position (0001)
        self_piece = BLACK_KING;
    }
    uint8_t target_pos;
    uint8_t ray_length;
    uint8_t other_piece;
    uint8_t relation;
    uint8_t possible_pin_pos;
    
    // Pinned pieces mask calculation and sliding pieces checks
    for (int i = 0; i<4; i++) // Check for orthogonal pins
    {
        possible_pin_pos = 0;
        ray_length = 0;
        target_pos = getOffsetPosition(king_pos, ORTHOGONAL_OFFSETS[i][0], ORTHOGONAL_OFFSETS[i][1]);
        other_piece = getPiece(state, target_pos);
        relation = getRelationToSelf(self_piece, other_piece);
        while (relation != ENEMY && target_pos != king_pos) // Goes forward until it hits an enemy or an edge
        {
            if (relation == FRIEND)
            {
                if (possible_pin_pos == 0) // If it passes a friendly piece, stores it as a potential pin
                {
                    possible_pin_pos = other_piece;
                }
                else // If it passes two friendly pieces, then there's no pin and it stops looking
                {
                    possible_pin_pos = 0;
                    break;
                }
            }
            ray_length++;
            target_pos = getOffsetPosition(king_pos, ORTHOGONAL_OFFSETS[i][0]*ray_length, ORTHOGONAL_OFFSETS[i][1]*ray_length);
            other_piece = getPiece(state, target_pos);
            relation = getRelationToSelf(self_piece, other_piece);
        }
        if (target_pos != king_pos && (other_piece == BLACK_QUEEN || other_piece == WHITE_QUEEN || other_piece == BLACK_ROOK || other_piece == WHITE_ROOK))
        {
            if (possible_pin_pos != 0) // If there is a pinned piece orthogonally
            {
                *out_pinned_pieces_mask |= (1ULL << possible_pin_pos); // Adds the pin to the mask
            }
            else if (relation == ENEMY) // If there is a direct orthogonal attacker
            {
                *out_attackers_mask |= (1ULL << target_pos); // Adds the attacker to the mask
            }
        }
    }
    for (int i = 0; i<4; i++) // Check for diagonal pins
    {
        possible_pin_pos = 0;
        ray_length = 0;
        target_pos = getOffsetPosition(king_pos, DIAGONAL_OFFSETS[i][0], DIAGONAL_OFFSETS[i][1]);
        other_piece = getPiece(state, target_pos);
        relation = getRelationToSelf(self_piece, other_piece);
        while (relation != ENEMY && target_pos != king_pos) // Goes forward until it hits an enemy or an edge
        {
            if (relation == FRIEND)
            {
                if (possible_pin_pos == 0) // If it passes a friendly piece, stores it as a potential pin
                {
                    possible_pin_pos = other_piece;
                }
                else // If it passes two friendly pieces, then there's no pin and it stops looking
                {
                    possible_pin_pos = 0;
                    break;
                }
            }
            ray_length++;
            target_pos = getOffsetPosition(king_pos, DIAGONAL_OFFSETS[i][0]*ray_length, DIAGONAL_OFFSETS[i][1]*ray_length);
            other_piece = getPiece(state, target_pos);
            relation = getRelationToSelf(self_piece, other_piece);
        }
        if (target_pos != king_pos && (other_piece == BLACK_QUEEN || other_piece == WHITE_QUEEN || other_piece == BLACK_BISHOP || other_piece == WHITE_BISHOP))
        {
            if (possible_pin_pos != 0) // If there is a pinned piece diagonally
            {
                *out_pinned_pieces_mask |= (1ULL << possible_pin_pos); // Adds the pin to the mask
            }
            else if (relation == ENEMY) // If there is a direct diagonal attacker
            {
                *out_attackers_mask |= (1ULL << target_pos); // Adds the attacker to the mask
            }
        }
    }

    // Get all attackers (except for the rooks, bishops and queens). So pawns and knights
}

Move *getAllValidBoardMoves(BoardState *state, int *move_count)
{
    Move *all_valid_moves = (Move*)malloc(MAX_MOVES * sizeof(Move));
    uint8_t white_to_move = (state->metadata) & WHITE_TO_MOVE_FLAG;
    uint64_t color_mask = 0ULL;
    if (white_to_move)
    {
        color_mask = UINT64_MAX;
    }

    uint64_t occupied = state->board[0] | state->board[1] | state->board[2] | state->board[3]; // Gets all positions with pieces
    uint64_t friendly_pieces = occupied & ~(state->board[0]^(color_mask)); // Gets all friendly pieces positions
    while (friendly_pieces != 0)
    {
        uint8_t piece_pos = bitscanForward(friendly_pieces);
        getAllPseudoValidPieceMoves(state, piece_pos, all_valid_moves, move_count);
        friendly_pieces &= (friendly_pieces-1);
    }

    return all_valid_moves;
}