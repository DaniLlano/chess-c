#include <stdio.h>
#include <stdlib.h>

// piece encoding: bits 0-2 = type, bit 3 = color
typedef enum {
    EMPTY  = 0,
    PAWN   = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK   = 4,
    QUEEN  = 5,
    KING   = 6,

    BLACK  = 8   // color flag: OR'd with type bits
} Piece;

// masks to extract piece info from a raw int
#define TYPE_MASK  0x07   // 0b00000111
#define COLOR_MASK 0x08   // 0b00001000

// castling rights flags (combinable with |)
#define CASTLE_WK  0x01   // white kingside
#define CASTLE_WQ  0x02   // white queenside
#define CASTLE_BK  0x04   // black kingside
#define CASTLE_BQ  0x08   // black queenside

// move flags
#define FLAG_NONE      0x00
#define FLAG_ENPASSANT 0x01
#define FLAG_CASTLE    0x02
#define FLAG_PROMOTE   0x04

typedef struct {
    int from;
    int to;
    int flags;
} Move;

#define MAX_MOVES 256

void init_board(void);
void init_game(void);
void print_board(void);
void print_state(void);
char piece_char(int piece);
void generate_moves(void);
void print_move_count(void);

typedef struct {
    int side_to_move;     // WHITE (0) or BLACK (8)
    int castle_rights;    // bitmask: CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ
    int en_passant;       // target square idx, or -1 if none
    int halfmove_clock;
    int fullmove_number;
} GameState;

GameState state;

Move moves[MAX_MOVES];
int  move_count = 0;

void init_game(void) {
    init_board();

    state.side_to_move    = 0;
    state.castle_rights   = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ;
    state.en_passant      = -1;
    state.halfmove_clock  = 0;
    state.fullmove_number = 1;
}

// board[0] = a8 (black's back rank, left), board[63] = h1
int board[64];

void init_board(void) {
    // clear all squares
    for (int i = 0; i < 64; i++) {
        board[i] = EMPTY;
    }

    // black pieces (rank 8, idx 0-7)
    board[0] = BLACK + ROOK;
    board[1] = BLACK + KNIGHT;
    board[2] = BLACK + BISHOP;
    board[3] = BLACK + QUEEN;
    board[4] = BLACK + KING;
    board[5] = BLACK + BISHOP;
    board[6] = BLACK + KNIGHT;
    board[7] = BLACK + ROOK;

    // black pawns (rank 7, idx 8-15)
    for (int i = 8; i < 16; i++) {
        board[i] = BLACK + PAWN;
    }

    // white pawns (rank 2, idx 48-55)
    for (int i = 48; i < 56; i++) {
        board[i] = PAWN;
    }

    // white pieces (rank 1, idx 56-63)
    board[56] = ROOK;
    board[57] = KNIGHT;
    board[58] = BISHOP;
    board[59] = QUEEN;
    board[60] = KING;
    board[61] = BISHOP;
    board[62] = KNIGHT;
    board[63] = ROOK;
}

// returns the display character for a given piece value
char piece_char(int piece) {
    int type  = piece & TYPE_MASK;
    int color = piece & COLOR_MASK;

    char symbols[] = {'.', 'P', 'N', 'B', 'R', 'Q', 'K'};
    char c = symbols[type];

    // uppercase = white, lowercase = black
    if (color == BLACK && c != '.') {
        c = c + 32;   // ASCII offset between 'A' and 'a'
    }
    return c;
}

void print_board(void) {
    printf("  a b c d e f g h\n");
    for (int rank = 7; rank >= 0; rank--) {
        printf("%d ", rank + 1);
        for (int col = 0; col < 8; col++) {
            int idx = (7 - rank) * 8 + col;  // invert rank for display
            printf("%c ", piece_char(board[idx]));
        }
        printf("\n");
    }
}

void print_state(void) {
    printf("side to move : %s\n", state.side_to_move == 0 ? "white" : "black");

    printf("castle rights: %c%c%c%c\n",
        (state.castle_rights & CASTLE_WK) ? 'K' : '-',
        (state.castle_rights & CASTLE_WQ) ? 'Q' : '-',
        (state.castle_rights & CASTLE_BK) ? 'k' : '-',
        (state.castle_rights & CASTLE_BQ) ? 'q' : '-'
    );

    if (state.en_passant == -1)
        printf("en passant   : -\n");
    else
        printf("en passant   : %d\n", state.en_passant);

    printf("halfmove     : %d\n", state.halfmove_clock);
    printf("fullmove     : %d\n", state.fullmove_number);
}

static int is_enemy(int piece, int side) {
    if (piece == EMPTY) return 0;
    return (piece & COLOR_MASK) != side;
}

static int is_empty_sq(int sq) {
    return board[sq] == EMPTY;
}

static void push_move(int from, int to, int flags) {
    if (move_count >= MAX_MOVES) return;
    moves[move_count].from  = from;
    moves[move_count].to    = to;
    moves[move_count].flags = flags;
    move_count++;
}

void generate_moves(void) {
    move_count = 0;
    int side = state.side_to_move;

    int rook_dirs[]   = { -8, -1,  1,  8 };
    int bishop_dirs[] = { -9, -7,  7,  9 };

    for (int sq = 0; sq < 64; sq++) {
        int piece = board[sq];
        if (piece == EMPTY) continue;
        if ((piece & COLOR_MASK) != side) continue;

        int type = piece & TYPE_MASK;
        int file = sq % 8;

        if (type == PAWN) {
            int dir   = (side == 0) ? -8 : 8;
            int start = (side == 0) ?  6 : 1;
            int rank  = sq / 8;

            int fwd = sq + dir;
            if (fwd >= 0 && fwd < 64 && is_empty_sq(fwd)) {
                push_move(sq, fwd, FLAG_NONE);
                int fwd2 = sq + dir * 2;
                if (rank == start && is_empty_sq(fwd2))
                    push_move(sq, fwd2, FLAG_NONE);
            }

            int cap_files[] = { file - 1, file + 1 };
            int cap_dirs[]  = { dir - 1,  dir + 1  };
            for (int i = 0; i < 2; i++) {
                if (cap_files[i] < 0 || cap_files[i] > 7) continue;
                int to = sq + cap_dirs[i];
                if (to < 0 || to >= 64) continue;
                if (is_enemy(board[to], side))
                    push_move(sq, to, FLAG_NONE);
                if (to == state.en_passant)
                    push_move(sq, to, FLAG_ENPASSANT);
            }
        }

        else if (type == KNIGHT) {
            int deltas[]     = { -17, -15, -10, -6,  6, 10, 15, 17 };
            int file_diffs[] = {  -1,   1,  -2,  2, -2,  2, -1,  1 };
            for (int i = 0; i < 8; i++) {
                int to = sq + deltas[i];
                if (to < 0 || to >= 64) continue;
                if (file + file_diffs[i] < 0 || file + file_diffs[i] > 7) continue;
                if (is_empty_sq(to) || is_enemy(board[to], side))
                    push_move(sq, to, FLAG_NONE);
            }
        }

        else if (type == ROOK || type == BISHOP || type == QUEEN) {
            int *dirs[2];
            int  ndirs[2] = { 0, 0 };

            if (type == ROOK)   { dirs[0] = rook_dirs;   ndirs[0] = 4; }
            if (type == BISHOP) { dirs[0] = bishop_dirs; ndirs[0] = 4; }
            if (type == QUEEN)  {
                dirs[0] = rook_dirs;   ndirs[0] = 4;
                dirs[1] = bishop_dirs; ndirs[1] = 4;
            }

            for (int s = 0; s < 2; s++) {
                for (int d = 0; d < ndirs[s]; d++) {
                    int to   = sq + dirs[s][d];
                    int prev = sq;
                    while (to >= 0 && to < 64) {
                        if (abs((to % 8) - (prev % 8)) > 1) break;
                        if (is_empty_sq(to)) {
                            push_move(sq, to, FLAG_NONE);
                        } else {
                            if (is_enemy(board[to], side))
                                push_move(sq, to, FLAG_NONE);
                            break;
                        }
                        prev = to;
                        to  += dirs[s][d];
                    }
                }
            }
        }

        else if (type == KING) {
            int deltas[]     = { -9, -8, -7, -1,  1,  7,  8,  9 };
            int file_diffs[] = { -1,  0,  1, -1,  1, -1,  0,  1 };
            for (int i = 0; i < 8; i++) {
                int to = sq + deltas[i];
                if (to < 0 || to >= 64) continue;
                if (file + file_diffs[i] < 0 || file + file_diffs[i] > 7) continue;
                if (is_empty_sq(to) || is_enemy(board[to], side))
                    push_move(sq, to, FLAG_NONE);
            }
        }
    }
}

void print_move_count(void) {
    generate_moves();
    printf("legal moves  : %d\n", move_count);
}

int main(void) {
    init_game();
    print_board();
    print_state();
    print_move_count();
    return 0;
}