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
#include <math.h>

#include <sstream>

/**
 *
 */
int main(int argc, char** argv) {

	g_ui_open_status open_stat = g_ui::open();

	if (open_stat == G_UI_OPEN_STATUS_SUCCESSFUL) {
		g_canvas* canvas = g_canvas::create();
		g_ui::register_desktop_canvas(canvas);
		canvas->setBounds(g_rectangle(0, 0, 200, 200));

		// canvas test
		while (canvas->getBuffer().buffer == 0) {
			klog("no buffer yet, waiting");
			g_sleep(1000);
			canvas->setBounds(g_rectangle(0, 0, 200, 200));
		}

		klog("retrieving buffer");
		auto bufferInfo = canvas->getBuffer();
		klog("got buffer: %x, %i %i", bufferInfo.buffer, bufferInfo.width, bufferInfo.height);

		// get the surface ready and go:
		cairo_surface_t* bufferSurface = cairo_image_surface_create_for_data((uint8_t*) bufferInfo.buffer, CAIRO_FORMAT_ARGB32, bufferInfo.width,
				bufferInfo.height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bufferInfo.width));
		auto cr = cairo_create(bufferSurface);

		// paint background black
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0.3, 0.4, 0.6, 1);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);
		cairo_restore(cr);

		// draw a red circle
		cairo_set_source_rgb(cr, 1, 0, 0);
		cairo_arc(cr, 100, 100, 50, 0, 2 * M_PI);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_fill(cr);

		canvas->blit(g_rectangle(0, 0, bufferInfo.width, bufferInfo.height));

		uint8_t blocker = true;
		g_atomic_block(&blocker);
	}
}
