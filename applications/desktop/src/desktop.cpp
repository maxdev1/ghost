/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *                  Tetris for the Ghost operating system                    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <desktop.hpp>
#include <ghostuser/ui/window.hpp>
#include <ghostuser/ui/label.hpp>
#include <ghostuser/ui/canvas.hpp>
#include <ghostuser/ui/button.hpp>
#include <ghostuser/ui/textfield.hpp>
#include <ghostuser/ui/action_listener.hpp>
#include <ghostuser/ui/mouse_listener.hpp>
#include <ghostuser/tasking/lock.hpp>
#include <math.h>
#include <iostream>
#include <list>
#include <sstream>

#include "component.hpp"
#include "taskbar.hpp"

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
int main(int argc, char** argv) {
	desktop_t desktop;
	desktop.run();
}

/**
 *
 */
class desktop_mouse_listener_t: public g_mouse_listener {
public:
	desktop_t* desktop;

	desktop_mouse_listener_t(desktop_t* desktop) :
			desktop(desktop) {
	}

	virtual void handle_mouse_event(g_ui_component_mouse_event* e) {
		desktop->cursor_position = e->position;
		desktop->handle_mouse_event(e);
		desktop->paint_uptodate = false;
	}
};

/**
 *
 */
void desktop_t::run() {

	g_ui_open_status open_stat = g_ui::open();

	if (open_stat == G_UI_OPEN_STATUS_SUCCESSFUL) {

		// Initialize canvas
		canvas = g_canvas::create();
		g_ui::register_desktop_canvas(canvas);
		g_ui::get_screen_dimension(&screen_dim);

		canvas->setBounds(g_rectangle(0, 0, screen_dim.width, screen_dim.height));
		canvas->setMouseListener(new desktop_mouse_listener_t(this));

		// Create taskbar
		taskbar = new taskbar_t(this);
		components.push_back(taskbar);

		// Main loop
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

			// draw components
			for (component_t* component : components) {
				component->paint(cr);
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
void desktop_t::handle_mouse_event(g_ui_component_mouse_event* e) {

	auto iter = components.rbegin();
	for (; iter != components.rend(); ++iter) {
		auto component = *iter;

		if (component->bounds.contains(e->position)) {
			e->position = e->position - component->bounds.getStart();
			component->handle_mouse_event(e);
			break;
		}
	}
}

/**
 *
 */
cairo_t* desktop_t::get_drawing_context() {

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
