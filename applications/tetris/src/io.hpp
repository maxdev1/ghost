//  Tetris implementation for ghostkernel

#include <board.hpp>
#include <ghostuser/ui/canvas.hpp>
#include <math.h>
#include <cairo/cairo.h>

void draw_rectangle(int x, int y, cairo_t *screen);

void draw_tetr(int x_b, int y_b, int tetr[5][5],  cairo_t *screen);

void draw_board(int x_b, int y_b, int board[WIDTH][HEIGHT],  cairo_t *screen);

