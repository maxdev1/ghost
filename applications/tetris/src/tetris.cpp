/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *             Tetris game example for the Ghost operating system            *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <ghostuser/ui/ui.hpp>
#include <ghostuser/ui/window.hpp>
#include <ghostuser/ui/button.hpp>
#include <ghostuser/ui/label.hpp>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <board.hpp>
#include <tetramino.hpp>
#include <io.hpp>


#define PLACE_BUTTON_T(num, text, x, y) \
                but##num = g_button::create();  \
                but##num->setTitle(text);               \
                but##num->setBounds(g_rectangle(x, y, 50, 50)); \
                window->addChild(but##num);

#define PLACE_BUTTON(num, x, y) PLACE_BUTTON_T(num, #num, x, y);


#define WAIT_TIME 700

//global variables needs to be global for gui controls
int x;
int y;

int tetr[5][5];
int board[WIDTH][HEIGHT];

int rotation;
int num_tetr;

/**
 *
 */
void command_pressed(int command) {

        if (command == 1) {
                //Left
		if(IsPossMoveLeft(x, y, board, tetr)==0)
			x-=1;

        } else if (command == 2) {
                //Right
		if(IsPossMoveRight(x, y, board, tetr)==0)
	                x+=1;

        } else if (command == 3) {
                //Rotate
		int rotated_tetr[5][5];

		if(rotation!=3)
			next_tetr(num_tetr, rotation+1, rotated_tetr);
		else
			next_tetr(num_tetr, 0, rotated_tetr);

                if(IsPossRotate(x, y, num_tetr, rotation, board, rotated_tetr)==0)
                {

                        if(rotation!=3)
                        {
                                rotation +=1;
                                next_tetr(num_tetr, rotation, tetr);
                        }
                        else
                        {
                                rotation=0;
                                next_tetr(num_tetr, rotation, tetr);
                        }
                }

        } else if (command == 4) {
		//Down
                while(IsPossMoveDown(x, y, board, tetr)==0)
		{
			y+=1;
		}
	}
}

/**
 *
 */
class command_press_action_listener_t: public g_action_listener {
public:
        int command;

        command_press_action_listener_t(int com) {
                this->command = com;
        }

        virtual void handle_action() {
                command_pressed(command);
        }
};

/**
 * This example application creates a tetris game with drawing on canvas and gui buttons
 */
int main(int argc, char** argv) {

	// Open the UI
	g_ui_open_status open_stat = g_ui::open();
	if (open_stat != G_UI_OPEN_STATUS_SUCCESSFUL) {
		std::cerr << "User interface could not be initialized" << std::endl;
		return -1;
	}

	// Create a window
	g_window* window = g_window::create();
	window->setTitle("Tetris game");

	// Create the canvas
	g_canvas* canvas = g_canvas::create();
	window->addChild(canvas);

	// Resize the window
	g_rectangle windowBounds(100, 100, 460, 460);
	window->setBounds(windowBounds);
	window->setVisible(true);
	canvas->setBounds(g_rectangle(0, 0, windowBounds.width, windowBounds.height));

	//Display for scores
	g_label* display;
	display = g_label::create();
	display->setTitle("score 0");
	display->setBounds(g_rectangle(300, 50, 150, 30));
	window->addChild(display);

	// Variables for our surface
	cairo_surface_t* surface = 0;
	uint8_t* surfaceBuffer = 0;

	// Create control buttons
	g_button* butLeft; //Left
	g_button* butRight; //Right
	g_button* butRotate; //Rotate
	g_button* butDown; //Down

        int gridy = 250;
	int gridy2 = 190;
	int gridy3 = 310;

        int grid1 = 300;
	int grid2 = 360;
        int grid3 = 330;
        int grid4 = 330;
        int dispOff = 40;

        PLACE_BUTTON_T(Left, "Left", grid1, gridy + dispOff);
        butLeft->setActionListener(new command_press_action_listener_t(1));
	PLACE_BUTTON_T(Right, "Right", grid2, gridy + dispOff);
        butRight->setActionListener(new command_press_action_listener_t(2));
        PLACE_BUTTON_T(Rotate, "Rotate", grid3, gridy2 + dispOff);
        butRotate->setActionListener(new command_press_action_listener_t(3));
	PLACE_BUTTON_T(Down, "Down", grid4, gridy3 + dispOff);
        butDown->setActionListener(new command_press_action_listener_t(4));


	//Create and initialize other game variables
	uint64_t time1 = g_millis();
        uint64_t time2;

	//Initianal speed
	int speed = 2;

	//Initial tetraminos position
	y = 0;
	x = 5;

	int score=0;

	int quit=0;

	int num_next_tetr;

	//Next tetramino
	int nextTetr[5][5];

	//Increase speed flag.
	int flag1=0;
	//GameOver flag
	int flag2=0;

	int i;

	//Random initialization
	srand(time(NULL));

	rotation = rand()%4;
	num_tetr = rand()%7;
	num_next_tetr = rand()%7;

	next_tetr(num_tetr, rotation, tetr);

	next_tetr(num_next_tetr, rotation, nextTetr);

	init_board(board);

	while (true) {
		// Resize canvas when window is resized
		auto windowBounds = window->getBounds();
		if (canvas->getBounds() != windowBounds) {
			canvas->setBounds(g_rectangle(0, 0, windowBounds.width, windowBounds.height));
		}

		// Get buffer
		auto bufferInfo = canvas->getBuffer();
		if (bufferInfo.buffer == 0) {
			g_sleep(100);
			continue;
		}

		// Get the surface ready and go:
		if (surface == 0 || surfaceBuffer != bufferInfo.buffer) {
			surface = cairo_image_surface_create_for_data((uint8_t*) bufferInfo.buffer, CAIRO_FORMAT_ARGB32, bufferInfo.width, bufferInfo.height,
					cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bufferInfo.width));
			surfaceBuffer = bufferInfo.buffer;
		}

		// Perform the painting
		auto cr = cairo_create(surface);

		// Clear the background
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0, 0, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);
		cairo_restore(cr);

		cairo_set_line_width(cr, 2.0);
		cairo_set_source_rgb(cr, 1, 0.6, 0.6);

		uint64_t time2 = g_millis();

		if((time2-time1) > (WAIT_TIME-100*speed))
                {
                        //Move Down
                        if(IsPossMoveDown(x, y, board, tetr)!=0)
                        {
                                Store_tetr(x, y, board, tetr);
                                y=0;
                                x=5;
                                num_tetr=num_next_tetr;
                                num_next_tetr=rand()%7;
                                next_tetr(num_tetr, rotation, tetr);
                                next_tetr(num_next_tetr, rotation, nextTetr);

                                score+=delete_line(board);
				std::stringstream ss;
				ss << score;
				display->setTitle("score " +ss.str());

				if(((score%10)==0)&& score!=0)
                                {
                                        if((speed<6) && flag1==0)
                                        {
                                                flag1=1;
                                                speed+=1;
                                        }
                                }
                                else
                                {
                                        flag1=0;
                                }

			}
                        y+=1;
                        time1 = g_millis();

                }
		else
                {
                        for(i=0; i<WIDTH; i++)
                                        {
                                                if(board[i][3]==1)
                                                {
                                                        flag2=1;
                                                }
                                        }

                }

		draw_tetr(x, y, tetr, cr);

		//draw next tetramino
		draw_tetr(14, 4, nextTetr, cr);

		draw_board(0, 0, board, cr);

		if(flag2==1)
		{
			display->setTitle("GAME OVER");
		}

		// Blit the content to screen
		canvas->blit(g_rectangle(0, 0, bufferInfo.width, bufferInfo.height));

		//g_sleep(100);
	}
}
