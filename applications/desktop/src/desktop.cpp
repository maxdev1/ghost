/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *                  Tetris for the Ghost operating system                    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <desktop.hpp>
#include <ghostuser/ui/ui.hpp>
#include <ghostuser/ui/window.hpp>
#include <ghostuser/ui/label.hpp>
#include <ghostuser/ui/canvas.hpp>
#include <ghostuser/ui/button.hpp>
#include <ghostuser/ui/textfield.hpp>
#include <ghostuser/ui/action_listener.hpp>
#include <ghostuser/tasking/lock.hpp>
#include <math.h>

#include <sstream>

static g_lock updateLock;


/**
 *
 */
int main(int argc, char** argv) {

	g_ui_open_status open_stat = g_ui::open();

	if (open_stat == G_UI_OPEN_STATUS_SUCCESSFUL) {
		g_canvas* canvas = g_canvas::create();
		g_ui::register_desktop_canvas(canvas);
		canvas->setBounds(g_rectangle(0, 0, 800, 600));

		int x = 0;
		int y = 0;
		while (true) {
			auto bufferInfo = canvas->getBuffer();
			if (bufferInfo.buffer == 0) {
				g_sleep(100);
				continue;
			}

			// get the surface ready and go:
			cairo_surface_t* bufferSurface = cairo_image_surface_create_for_data((uint8_t*) bufferInfo.buffer, CAIRO_FORMAT_ARGB32, bufferInfo.width,
					bufferInfo.height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bufferInfo.width));
			auto cr = cairo_create(bufferSurface);

			// clear
			cairo_save(cr);
			cairo_set_source_rgba(cr, 0, 0, 0, 0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);
			cairo_restore(cr);

			// draw a red circle
			cairo_set_source_rgb(cr, 1, 0, 0);
			cairo_arc(cr, x, y, 50, 0, 2 * M_PI);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_fill(cr);

			x += 5;
			if (x > 800) {
				x = 0;
				y += 5;
			}

			canvas->blit(g_rectangle(0, 0, bufferInfo.width, bufferInfo.height));

			g_sleep(10);
		}
	}
}
