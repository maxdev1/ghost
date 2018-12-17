#include "taskbar.hpp"

/**
 *
 */
taskbar_t::taskbar_t(desktop_t* desktop) :
		component_t(desktop) {
	start_button_layout = g_text_layouter::getInstance()->initializeBuffer();
	font = g_font_loader::getDefault();

	bounds = g_rectangle(0, desktop->screen_dim.height - TASKBAR_HEIGHT, desktop->screen_dim.width, TASKBAR_HEIGHT);
	start_bounds = g_rectangle(20, bounds.y, 100, bounds.height);
}

/**
 *
 */
void taskbar_t::paint(cairo_t* cr) {
	cairo_save(cr);
	cairo_set_source_rgba(cr, 0.1, 0.1, 0.15, 0.8);
	cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
	cairo_fill(cr);
	cairo_restore(cr);

	// start button

	cairo_save(cr);
	if (start_bounds.contains(desktop->cursor_position)) {
		cairo_set_source_rgba(cr, 0.3, 0.3, 0.32, 0.8);
	} else {
		cairo_set_source_rgba(cr, 0.3, 0.3, 0.32, 0.5);
	}
	cairo_rectangle(cr, start_bounds.x, start_bounds.y, start_bounds.width, start_bounds.height);
	cairo_fill(cr);
	cairo_restore(cr);

	// Paint glyphs
	g_text_layouter::getInstance()->layout(cr, "Terminal", font, 14, start_bounds, g_text_alignment::CENTER, start_button_layout);

	for (g_positioned_glyph& g : start_button_layout->positions) {
		cairo_save(cr);
		if (start_bounds.contains(desktop->cursor_position)) {
			cairo_set_source_rgba(cr, 1, 1, 1, 1);
		} else {
			cairo_set_source_rgba(cr, 1, 1, 1, 0.8);
		}
		cairo_translate(cr, g.position.x - g.glyph->x + 5, g.position.y - g.glyph->y + 5);
		cairo_glyph_path(cr, g.glyph, g.glyph_count);
		cairo_fill(cr);
		cairo_restore(cr);
	}

}

/**
 *
 */
void spawn_terminal() {
	g_spawn("/applications/terminal2.bin", "", "/", G_SECURITY_LEVEL_APPLICATION);
}

/**
 *
 */
void taskbar_t::handle_mouse_event(g_ui_component_mouse_event* e) {

	static bool startPressed = false;

	if (e->type == G_MOUSE_EVENT_PRESS) {
		if (start_bounds.contains(desktop->cursor_position)) {
			startPressed = true;
		}
	}
	if (e->type == G_MOUSE_EVENT_RELEASE) {
		if (start_bounds.contains(desktop->cursor_position)) {
			g_create_thread((void*) spawn_terminal);
		}
	}

}
