#include <stdio.h>

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

void init_board(void);
void init_game(void);
void print_board(void);
void print_state(void);
char piece_char(int piece);

typedef struct {
    int side_to_move;     // WHITE (0) or BLACK (8)
    int castle_rights;    // bitmask: CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ
    int en_passant;       // target square idx, or -1 if none
    int halfmove_clock;
    int fullmove_number;
} GameState;

GameState state;

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

int main(void) {
    init_game();
    print_board();
    print_state();
    return 0;
}