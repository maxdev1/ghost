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
#include <ghostuser/ui/mouse_listener.hpp>
#include <ghostuser/graphics/text/text_layouter.hpp>
#include <ghostuser/graphics/text/font_loader.hpp>
#include <ghostuser/tasking/lock.hpp>
#include <math.h>
#include <iostream>
#include <list>
#include <sstream>

/**
 *
 */
g_atom paint_uptodate = false;
g_point cursor_position;

g_dimension screen_dim;

/**
 * Variables used for the status of the canvas and its buffer.
 */
g_canvas* canvas;
g_dimension canvas_current_buffer_dimension;
cairo_t* canvas_current_context;
cairo_surface_t* canvas_current_surface = 0;
uint8_t* canvas_current_buffer = 0;
g_canvas_buffer_info canvas_buffer_info;

/**
 *
 */
void spawn_terminal() {
	g_spawn("/applications/terminal2.bin", "", "/", G_SECURITY_LEVEL_APPLICATION);
}

/**
 *
 */
class desktop_mouse_listener_t: public g_mouse_listener {
public:
	virtual void handle_mouse_event(g_ui_component_mouse_event* e) {
		cursor_position = e->position;
		paint_uptodate = false;

		static bool startPressed = false;

		if (e->type == G_MOUSE_EVENT_PRESS) {
			if (cursor_position.x < 120 && cursor_position.y > screen_dim.height - 30) {
				startPressed = true;
			}
		}
		if (e->type == G_MOUSE_EVENT_RELEASE) {
			if (cursor_position.x < 120 && cursor_position.y > screen_dim.height - 30 && startPressed) {
				g_create_thread((void*) spawn_terminal);
			}
		}
	}
};

/**
 *
 */
int main(int argc, char** argv) {

	g_ui_open_status open_stat = g_ui::open();

	if (open_stat == G_UI_OPEN_STATUS_SUCCESSFUL) {
		canvas = g_canvas::create();
		g_ui::register_desktop_canvas(canvas);

		g_ui::get_screen_dimension(&screen_dim);

		canvas->setBounds(g_rectangle(0, 0, screen_dim.width, screen_dim.height));
		canvas->setMouseListener(new desktop_mouse_listener_t());

		int taskbar_height = 30;

		auto start_button_layout = g_text_layouter::getInstance()->initializeBuffer();
		g_font* font = g_font_loader::getDefault();

		while (true) {
			// get buffer
			auto cr = get_drawing_context();
			if (cr == 0) {
				g_sleep(100);
				continue;
			}

			// clear
			cairo_save(cr);
			cairo_set_source_rgba(cr, 0, 0, 0, 0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);
			cairo_restore(cr);

			// draw task bar
			cairo_save(cr);
			cairo_set_source_rgba(cr, 0.1, 0.1, 0.15, 0.8);
			cairo_rectangle(cr, 0, screen_dim.height - taskbar_height, screen_dim.width, taskbar_height);
			cairo_fill(cr);
			cairo_restore(cr);

			// start button
			g_rectangle start_bounds(20, screen_dim.height - taskbar_height, 100, taskbar_height);

			cairo_save(cr);
			if (start_bounds.contains(cursor_position)) {
				cairo_set_source_rgba(cr, 0.3, 0.3, 0.32, 0.8);
			} else {
				cairo_set_source_rgba(cr, 0.3, 0.3, 0.32, 0.5);
			}
			cairo_rectangle(cr, start_bounds.x, start_bounds.y, start_bounds.width, start_bounds.height);
			cairo_fill(cr);
			cairo_restore(cr);

			g_text_layouter::getInstance()->layout(cr, "Terminal", font, 14, start_bounds, g_text_alignment::CENTER, start_button_layout);

			// Paint glyphs
			for (g_positioned_glyph& g : start_button_layout->positions) {
				cairo_save(cr);
				if (start_bounds.contains(cursor_position)) {
					cairo_set_source_rgba(cr, 1, 1, 1, 1);
				} else {
					cairo_set_source_rgba(cr, 1, 1, 1, 0.8);
				}
				cairo_translate(cr, g.position.x - g.glyph->x + 5, g.position.y - g.glyph->y + 5);
				cairo_glyph_path(cr, g.glyph, g.glyph_count);
				cairo_fill(cr);
				cairo_restore(cr);
			}

			// redraw entire screen (TODO: only redraw partially)
			canvas->blit(g_rectangle(0, 0, canvas_buffer_info.width, canvas_buffer_info.height));

			paint_uptodate = true;
			g_atomic_block_to(&paint_uptodate, 1000);
		}
	}
}

/**
 *
 */
cairo_t* get_drawing_context() {

	// get buffer
	canvas_buffer_info = canvas->getBuffer();
	if (canvas_buffer_info.buffer == 0) {
		return 0;
	}

	canvas_current_buffer_dimension.width = canvas_buffer_info.width;
	canvas_current_buffer_dimension.height = canvas_buffer_info.height;

	// get the surface ready and go:
	if (canvas_current_surface == 0 || canvas_current_buffer != canvas_buffer_info.buffer) {
		if (canvas_current_context != 0) {
			cairo_destroy(canvas_current_context);
		}

		canvas_current_surface = cairo_image_surface_create_for_data((uint8_t*) canvas_buffer_info.buffer, CAIRO_FORMAT_ARGB32, canvas_buffer_info.width,
				canvas_buffer_info.height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, canvas_buffer_info.width));
		canvas_current_buffer = canvas_buffer_info.buffer;
		canvas_current_context = cairo_create(canvas_current_surface);
	}

	return canvas_current_context;
}
