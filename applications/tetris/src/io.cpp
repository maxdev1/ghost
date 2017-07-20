//  Tetris implementation for ghostkernel

#include <io.hpp>

void draw_rectangle(int x, int y, cairo_t *screen)
{
	cairo_rectangle(screen, x, y, 20, 20);

	cairo_fill(screen);
}

void draw_tetr(int x_b, int y_b, int tetr[5][5],  cairo_t *screen)
{
	int i;
	int j;
	int a,b;
	int x = x_b*20;
	int y = y_b*20;

	for(i =0; i < 5; i++)
	{
		y= y_b*20;

		for(j = 0; j < 5; j++)
		{
			if(tetr[i][j]>0)
			{
				draw_rectangle(x, y, screen);
			}

			y+=20;
		}

		x+=20;
	}
}

void draw_board(int x_b, int y_b, int board[WIDTH][HEIGHT], cairo_t *screen)
{
	int i;
	int j;
	int x = x_b;
	int y = y_b;

	for (i = 0;i < WIDTH; i++)
	{
		y = y_b;

		for(j = 0;j < HEIGHT; j++)
		{

			if(board[i][j]== 1)
		        {
				draw_rectangle(x, y, screen);
		        }

			y += 20;
		}

		x += 20;
	}
}

