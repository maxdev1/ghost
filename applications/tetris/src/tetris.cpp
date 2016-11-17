/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *             Canvas example for the Ghost operating system                 *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <ghostuser/ui/ui.hpp>
#include <ghostuser/ui/window.hpp>
#include <ghostuser/ui/canvas.hpp>
#include <ghostuser/ui/textfield.hpp>
#include <iostream>
#include <math.h>
#include <cairo/cairo.h>

/**
 * This example application creates a window with a canvas and draws one of the
 * cairo samples on it.
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
	window->setTitle("Test Window");

	// Create the canvas
	g_canvas* canvas = g_canvas::create();
	window->addChild(canvas);

	// Resize the window
	g_rectangle windowBounds(200, 200, 270, 270);
	window->setBounds(windowBounds);
	window->setVisible(true);
	canvas->setBounds(g_rectangle(0, 0, windowBounds.width, windowBounds.height));

	// Variables for our surface
	cairo_surface_t* surface = 0;
	uint8_t* surfaceBuffer = 0;

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

		// Draw the example
		double xc = 128.0;
		double yc = 128.0;
		double radius = 100.0;
		double angle1 = 45.0 * (M_PI / 180.0);
		double angle2 = 180.0 * (M_PI / 180.0);

		cairo_set_line_width(cr, 10.0);
		cairo_arc_negative(cr, xc, yc, radius, angle1, angle2);
		cairo_stroke(cr);

		cairo_set_source_rgba(cr, 1, 0.2, 0.2, 0.6);
		cairo_set_line_width(cr, 6.0);

		cairo_arc(cr, xc, yc, 10.0, 0, 2 * M_PI);
		cairo_fill(cr);

		cairo_arc(cr, xc, yc, radius, angle1, angle1);
		cairo_line_to(cr, xc, yc);
		cairo_arc(cr, xc, yc, radius, angle2, angle2);
		cairo_line_to(cr, xc, yc);
		cairo_stroke(cr);

		// Blit the content to screen
		canvas->blit(g_rectangle(0, 0, bufferInfo.width, bufferInfo.height));

		g_sleep(100);
	}
}
