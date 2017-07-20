//  Tetris game implementation for ghostkernel


#include <board.hpp>


void init_board(int board[WIDTH][HEIGHT])
{
	int i,j;
	for (i = 0; i < WIDTH; i++)
	{
		for (j = 0; j < HEIGHT; j++)
		{
			board[i][j] = 0;
		}
	}
}

int IsPossMoveRight(int p_x, int p_y, int board[WIDTH][HEIGHT], int tetr[5][5])
{
	int i,j;
	for(i = 0;i <5; i++)
	{
		for(j = 0; j < 5; j++)
		{
			if(tetr[i][j] == 1 )
			{
				if(board[i+p_x+1][j+p_y]==1)
					return 1;
				if((p_x+i)>=WIDTH-1)
					return 1;

			}
		}
	}
	return 0;

}

int IsPossMoveLeft(int p_x, int p_y, int board[WIDTH][HEIGHT], int tetr[5][5])
{
	int i,j;
	for(i = 0;i <5; i++)
	{
		for(j = 0; j < 5; j++)
		{
			if(tetr[i][j] == 1 )
			{
				if(board[i+p_x-1][j+p_y]==1)
					return 1;
				if((p_x+i)<=0)
					return 1;
			}
		}
	}
	return 0;

}

int IsPossMoveDown(int p_x, int p_y,int board[WIDTH][HEIGHT], int tetr[5][5])
{
	int i,j;
	for(i = 0;i < 5; i++)
	{
		for(j = 0; j < 5; j++)
		{
			if(tetr[i][j] == 1 )
			{
				        if((j+p_y)==HEIGHT-1)
					{
						return 1;
					}
					if(board[i+p_x][j+p_y+1]==1)
					{
						return 1;
					}
			}

		}
	}
	return 0;
}

int IsPossRotate(int p_x, int p_y, int num_tetr, int rotation, int board[WIDTH][HEIGHT], int tetr[5][5])
{
	int i,j;

	for(i = 0;i < 5; i++)
	{
		for(j = 0; j < 5; j++)
		{
			if(tetr[i][j] == 1 )
			{
				if((j+p_y)==HEIGHT-1)
				{
					return 1;
				}

				if(board[i+p_x][j+p_y]==1)
				{
					return 1;
				}

				if((p_x+i)>=WIDTH-1)
				{
					return 1;
				}

			        if((p_x+i)<=0)
					return 1;

			}

		}
	}
	return 0;

}

int delete_line(int board[WIDTH][HEIGHT])
{
	int j,k,g,n,d,sc;
	sc=0;

	for (j = 0; j <HEIGHT; j++)
	{
		int i = 0;
		while (i < WIDTH)
		{
			if (board[i][j] == 0)
				break;
			else
			{
				i++;
			}
		}

		if (i == WIDTH)
		{
			for (g=0; g<WIDTH; g++)
			{
				for (k = j; k > 0; k--)
				{
					board[g][k] = board[g][k-1];
				}
			}

			sc++;
		}
	}
return sc;

}

void Store_tetr(int p_x, int p_y, int board[WIDTH][HEIGHT], int tetr[5][5])
{
	int i,j;

	for(i = 0; i < 5; i++)
	{
		for( j = 0; j < 5; j++)
		{
			if(tetr[i][j] == 1)
			{
				board[p_x+i][p_y+j] = 1;
			}
		}
	}
}


