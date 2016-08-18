/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *                  Tetris for the Ghost operating system                    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <ghostuser/ui/ui.hpp>
#include <ghostuser/ui/window.hpp>
#include <ghostuser/ui/label.hpp>
#include <ghostuser/ui/canvas.hpp>
#include <ghostuser/ui/button.hpp>
#include <ghostuser/ui/textfield.hpp>
#include <ghostuser/ui/action_listener.hpp>
#include <tetris.hpp>
#include <math.h>

#include <sstream>

/**
 *
 */
int main(int argc, char** argv) {

	g_ui_open_status open_stat = g_ui::open();

	if (open_stat == G_UI_OPEN_STATUS_SUCCESSFUL) {
		g_window* window = g_window::create();

		window->setTitle("Tetris");

		g_canvas* canvas = g_canvas::create();
		window->addChild(canvas);
		canvas->setBounds(window->getBounds());

		window->setBounds(g_rectangle(200, 200, 200, 200));
		window->setVisible(true);

		// canvas test
		double r = 0;
		while (true) {
			// set canvas bounds
			auto windowBounds = window->getBounds();
			canvas->setBounds(g_rectangle(0, 0, windowBounds.width, windowBounds.height));

			// get buffer
			auto bufferInfo = canvas->getBuffer();
			if (bufferInfo.buffer == 0) {
				continue;
			}

			// get the surface ready and go:
			cairo_surface_t* bufferSurface = cairo_image_surface_create_for_data((uint8_t*) bufferInfo.buffer, CAIRO_FORMAT_ARGB32, bufferInfo.width,
					bufferInfo.height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bufferInfo.width));
			auto cr = cairo_create(bufferSurface);

			// paint background black
			cairo_save(cr);
			cairo_set_source_rgba(cr, 0, 0, 0, 1);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);
			cairo_restore(cr);

			// draw a red circle
			cairo_set_source_rgb(cr, r, 1, 0);
			cairo_arc(cr, 100,  100, (windowBounds.height < windowBounds.width ? windowBounds.height / 3 : windowBounds.width / 3), 0, 2 * M_PI);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_fill(cr);

			r += 0.01;
			if(r > 1) {
				r = 0;
			}

			canvas->blit(g_rectangle(0, 0, bufferInfo.width, bufferInfo.height));
		}
	}
}
