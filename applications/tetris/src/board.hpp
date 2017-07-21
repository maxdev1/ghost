//  Tetris implementation for ghostkernel


/*board.h*/

#define HEIGHT 20
#define WIDTH 14

void init_board(int board[WIDTH][HEIGHT]);

int IsPossMoveRight(int p_x, int p_y, int board[WIDTH][HEIGHT], int tetr[5][5]);

int IsPossMoveLeft(int p_x, int p_y, int board[WIDTH][HEIGHT], int tetr[5][5]);

int IsPossMoveDown(int p_x, int p_y, int board[WIDTH][HEIGHT], int tetr[5][5]);

int IsPossRotate(int p_x, int p_y, int num_tetr, int rotation, int board[WIDTH][HEIGHT], int tetr[5][5]);

int delete_line(int board[WIDTH][HEIGHT]);

void Store_tetr(int p_x, int p_y, int board[WIDTH][HEIGHT], int tetr[5][5]);

