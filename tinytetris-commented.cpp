#include <ctime>
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <algorithm>

#include <curses.h>
#include <unistd.h>
#include <stdlib.h>

using i8 = int8_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

#define LEFT 'a'
#define RIGHT 'd'
#define UP 'w'
#define DOWN 's'

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

class Tetris {
public:
    Tetris(size_t row, size_t col)
        : board_(row, std::vector<u8>(col))
    {
        if (row == 0 || col == 0) {
            throw std::invalid_argument("row and column cannot be 0");
        }
        new_piece();
    }

    void process_input(char input) {
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
                break;
        }
    }

    bool do_tick() {
        static int tick;

        if (++tick < 30) return true;
        tick = 0;

        if (check_hit(x_, y_ + 1, r_)) {
            if (y_ == 0) return false;
            draw_piece();
            remove_line();
            new_piece();
            if (check_hit(x_, y_, r_)) return false;
        } else {
            y_++;
        }

        return true;
    }


public: // data access
    auto row() const {
        return board_.size();
    }
    auto col() const {
        return board_[0].size();
    }
    auto score() const {
        return score_;
    }
    u8 board(size_t row, size_t col) const {
        for (size_t i = 0; i < 4; i++) {
            auto x = x_ + p_getx(r_, i), y = y_ + p_gety(r_, i);
            if (row == y && col == x)
                return p_ + 1;
        }
        return board_[row][col];
    }

private:
    // extract a 2-bit number from a block entry
    u8 NUM(u8 r, u8 n) const { return (BLOCK[p_][r] >> n) & 3; }
    size_t p_width(u8 r) const { return NUM(r, 16) + 1; }
    size_t p_height(u8 r) const { return NUM(r, 18) + 1; }
    size_t p_getx(u8 r, size_t i) const { return NUM(r, i * 4 + 2); }
    size_t p_gety(u8 r, size_t i) const { return NUM(r, i * 4); }

    // create a new piece, don't remove old one (it has landed and should stick)
    void new_piece() {
        int rnum = rand();
        y_ = 0;
        p_ = rnum % 7;
        r_ = rnum % 4;
        x_ = rnum % (col() - p_width(r_) + 1);
    }

    // set the value of the board for a particular (x,y,r) piece
    void set_piece(size_t x, size_t y, size_t r, u8 v) {
        for (size_t i = 0; i < 4; i++) {
            board_[y + p_gety(r, i)][x + p_getx(r, i)] = v;
        }
    }

    // move a piece from old (p*) coords to new
    void draw_piece() {
        set_piece(x_, y_, r_, p_ + 1);
    }

    void erase_piece() {
        set_piece(x_, y_, r_, 0);
    }

    // remove line(s) from the board if they're full
    void remove_line() {
        for (u8 row = y_; row < y_ + p_height(r_); row++) {
            for (u8 i = 0; i < col(); i++) {
                if (!board_[row][i]) goto CONTINUE;
            }

            for (u8 i = row; i > 0; i--) {
                // memcpy(board_[i], board_[i - 1], sizeof(**board_) * col());
                std::copy(board_[i - 1].begin(), board_[i - 1].end(), board_[i].begin());
            }
            
            score_++;
            
            CONTINUE:;
        }
    }

    // check if placing p at (x,y,r) will be a collision
    u8 check_hit(i8 x, i8 y, u8 r) {
        if (y < 0 || y + p_height(r) > row()) return 1;
        if (x < 0 || x + p_width(r) > col()) return 1;

        for (u8 i = 0; i < 4; i++) {
            if (board_[y + p_gety(r, i)][x + p_getx(r, i)]) {
                return 1;
            }
        }

        return 0;
    }

    void move_piece(u8 flag) {
        i8 delta = ((flag == LEFT) ? -1 : 1);
        (void)(check_hit(x_ + delta, y_, r_) || (x_ += delta));
    }

    void rotate_piece(u8 flag) {
        i8 r = ((flag == LEFT)  ? LR(r_, 4) : RR(r_, 4));
        i8 x = x_;
        
        if (check_hit(x, y_, r) && 
        check_hit(x -= (p_width(r) - p_width(r_)), y_, r)) return;
        
        r_ = r;
        x_ = x;
    }
private:
    size_t x_, y_;
    size_t r_, p_;
    std::vector<std::vector<u8>> board_;
    uintmax_t score_ = 0;
};

/* draw the board and score */
static void frame(const Tetris& tetris) {
    for (int r = 0; r < tetris.row(); r++) {
        move(1 + r, 1); // otherwise the box won't draw
        for (int c = 0; c < tetris.col(); c++) {
            tetris.board(r, c) && attron(262176 | tetris.board(r, c) << 8);
            printw("  ");
            attroff(262176 | tetris.board(r, c) << 8);
        }
    }
    move(tetris.row() + 1, 1);
    printw("Score: %ju", tetris.score());
    refresh();
}


void runloop(Tetris&& tetris) {
	while (tetris.do_tick()) {
        usleep(10000);
        tetris.process_input(getch());

		frame(tetris);
	}
}

void curses_init() {
    srand(time(0));
    initscr();
    start_color();
    // colours indexed by their position in the block
    for (int i = 1; i < 8; i++) {
        init_pair(i, i, 0);
    }
    resizeterm(22, 22);
    noecho();
    timeout(0);
    curs_set(0);
    box(stdscr, 0, 0);
}

int main() {
    curses_init();
    Tetris tetris(20, 10);
    runloop(std::move(tetris));
    endwin();
}
