#include <ctime>
#include <cstdint>
#include <cstring>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>

using i8 = int8_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

#define BOARD_ROW 20
#define BOARD_COL 10

#define LEFT 'a'
#define RIGHT 'd'
#define UP 'w'
#define DOWN 's'

static i8 X, Y;
static u8 R, P;
static u16 SCORE;
static u8 BOARD[BOARD_ROW][BOARD_COL];

static constexpr const u32 BLOCK[7][4] = {
	{ 431424, 598356, 431424, 598356 },
	{ 427089, 615696, 427089, 615696 },
	{ 348480, 348480, 348480, 348480 },
	{ 599636, 431376, 598336, 432192 },
	{ 411985, 610832, 415808, 595540 },
	{ 247872, 799248, 247872, 799248 },
	{ 614928, 399424, 615744, 428369 },
};

static constexpr inline auto RR(auto d, auto n) { return (d + 1) % n; }
static constexpr inline auto LR(auto d, auto n) { return (d + n - 1) % n; }

// extract a 2-bit number from a block entry
static inline u8 NUM(u8 r, u8 n) { return (BLOCK[P][r] >> n) & 3; }
static inline i8 _width(u8 r) { return NUM(r, 16) + 1; }
static inline i8 _height(u8 r) { return NUM(r, 18) + 1; }
static inline i8 _getx(u8 r, u8 i) { return NUM(r, i * 4 + 2); }
static inline i8 _gety(u8 r, u8 i) { return NUM(r, i * 4); }

// create a new piece, don't remove old one (it has landed and should stick)
static void new_piece() {
	int rnum = rand();
	Y = 0;
	P = rnum % 7;
	R = rnum % 4;
	X = rnum % (BOARD_COL - _width(R) + 1);
}

// set the value of the board for a particular (x,y,r) piece
static void set_piece(i8 x, i8 y, u8 r, u8 v) {
	for (u8 i = 0; i < 4; i++) {
		BOARD[y + _gety(r, i)][x + _getx(r, i)] = v;
	}
}

static void erase_piece() {
	set_piece(X, Y, R, 0);
}

// move a piece from old (p*) coords to new
static void draw_piece() {
	set_piece(X, Y, R, P + 1);
}

// remove line(s) from the board if they're full
static void remove_line() {
	for (u8 row = Y; row < Y + _height(R); row++) {
		for (u8 i = 0; i < BOARD_COL; i++) {
			if (!BOARD[row][i]) goto CONTINUE;
		}

		for (u8 i = row; i > 0; i--) {
			memcpy(BOARD[i], BOARD[i - 1], sizeof(**BOARD) * BOARD_COL);
		}
		
		SCORE++;
		
		CONTINUE:;
	}
}

// check if placing p at (x,y,r) will be a collision
static u8 check_hit(i8 x, i8 y, u8 r) {
	if (y < 0 || y + _height(r) > BOARD_ROW) return 1;
	if (x < 0 || x + _width(r) > BOARD_COL) return 1;

	for (u8 i = 0; i < 4; i++) {
		if (BOARD[y + _gety(r, i)][x + _getx(r, i)]) {
			return 1;
		}
	}

	return 0;
}

static void rotate_piece(u8 flag) {
	i8 r = ((flag == LEFT)  ? LR(R, 4) : RR(R, 4));
	i8 x = X;
	
	if (check_hit(x, Y, r) && 
	check_hit(x -= (_width(r) - _width(R)), Y, r)) return;
	
	R = r;
	X = x;
}

static void move_piece(u8 flag) {
	i8 delta = ((flag == LEFT) ? -1 : 1);
	(void)(check_hit(X + delta, Y, R) || (X += delta));
}

/* draw the board and score */
static void frame() {
  for (int r = 0; r < BOARD_ROW; r++) {
    move(1 + r, 1); // otherwise the box won't draw
    for (int j = 0; j < BOARD_COL; j++) {
      BOARD[r][j] && attron(262176 | BOARD[r][j] << 8);
      printw("  ");
      attroff(262176 | BOARD[r][j] << 8);
    }
  }
  move(BOARD_ROW + 1, 1);
  printw("Score: %u", SCORE);
  refresh();
}

void tetris_process_input() {
	bool changed = true;
    auto input = getch();
    
    switch (input) {
        case LEFT:
        case RIGHT:
            move_piece(input);
            break;
        case UP:
            rotate_piece(RIGHT);
            break;
        case DOWN:
            rotate_piece(LEFT);
        default:
            changed = false;
    }
}

u8 tetris_do_tick() {
    static int tick;

    if (++tick < 30) return 1;
    tick = 0;

	if (check_hit(X, Y + 1, R)) {
		if (Y == 0) return 0;
		draw_piece();
		remove_line();
		new_piece();
		if (check_hit(X, Y, R)) return 0;
	} else {
		Y++;
	}

	draw_piece();
	frame();
	erase_piece();
	
	return 1;
}

static void runloop() {
	while (tetris_do_tick()) {
        usleep(10000);
        tetris_process_input();

		draw_piece();
		frame();
		erase_piece();
	}
}

void tetris_init() {	
    srand(time(0));
    initscr();
    start_color();
    // colours indexed by their position in the block
    for (int i = 1; i < 8; i++) {
    init_pair(i, i, 0);
    }
    new_piece();
    resizeterm(22, 22);
    noecho();
    timeout(0);
    curs_set(0);
    box(stdscr, 0, 0);
}

int main() {
    tetris_init();
    runloop();
    endwin();
}
