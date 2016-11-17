/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <components/component.hpp>
#include <components/window.hpp>

#include <events/locatable.hpp>
#include <events/key_event.hpp>
#include <events/mouse_event.hpp>
#include <windowserver.hpp>

#include <ghostuser/utils/logger.hpp>
#include <ghostuser/ui/properties.hpp>
#include <algorithm>
#include <cairo/cairo.h>

#include <layout/flow_layout_manager.hpp>
#include <layout/grid_layout_manager.hpp>

/**
 *
 */
component_t::~component_t() {
	if (layoutManager) {
		delete layoutManager;
	}
}

/**
 *
 */
void component_t::setBounds(const g_rectangle& newBounds) {
	g_rectangle oldBounds = bounds;

	markDirty();
	bounds = newBounds;
	if (bounds.width < minimumSize.width) {
		bounds.width = minimumSize.width;
	}
	if (bounds.height < minimumSize.height) {
		bounds.height = minimumSize.height;
	}
	markDirty();

	if (oldBounds.width != bounds.width || oldBounds.height != bounds.height) {
		graphics.resize(bounds.width, bounds.height);
		markFor(COMPONENT_REQUIREMENT_LAYOUT);
		markFor(COMPONENT_REQUIREMENT_UPDATE);
		markFor(COMPONENT_REQUIREMENT_PAINT);

		handleBoundChange(oldBounds);
	}

	fireBoundsChange(bounds);
}

/**
 *
 */
bool component_t::canHandleEvents() const {

	if (!visible) {
		return false;
	}

	if (parent) {
		return parent->canHandleEvents();
	}

	return true;
}

/**
 *
 */
void component_t::setVisible(bool visible) {
	this->visible = visible;
	markDirty();
	markFor(COMPONENT_REQUIREMENT_ALL);
}

/**
 *
 */
void component_t::markDirty(g_rectangle rect) {

	if (parent) {
		rect.x += bounds.x;
		rect.y += bounds.y;
		parent->markDirty(rect);
	}

}

/**
 *
 */
void component_t::blit(g_graphics* out, g_rectangle absClip, g_point position) {

	if (this->visible) {
		if (graphics.getContext() != 0) {
			graphics.blitTo(out, absClip, position);
		}

		g_rectangle ownAbsBounds = getBounds();
		ownAbsBounds.x = position.x;
		ownAbsBounds.y = position.y;
		int newTop = absClip.getTop() > ownAbsBounds.getTop() ? absClip.getTop() : ownAbsBounds.getTop();
		int newBottom = absClip.getBottom() < ownAbsBounds.getBottom() ? absClip.getBottom() : ownAbsBounds.getBottom();
		int newLeft = absClip.getLeft() > ownAbsBounds.getLeft() ? absClip.getLeft() : ownAbsBounds.getLeft();
		int newRight = absClip.getRight() < ownAbsBounds.getRight() ? absClip.getRight() : ownAbsBounds.getRight();

		g_rectangle thisClip = g_rectangle(newLeft, newTop, newRight - newLeft, newBottom - newTop);
		for (component_t* c : children) {
			if (c->visible) {
				c->blit(out, thisClip, g_point(position.x + c->bounds.x, position.y + c->bounds.y));
			}
		}
	}
}

/**
 *
 */
void component_t::addChild(component_t* comp) {

	if (comp->parent) {
		comp->parent->removeChild(comp);
	}

	children.push_back(comp);

	std::sort(children.begin(), children.end(), [](component_t*& struct1, component_t*& struct2) {
		return struct1->z_index < struct2->z_index;
	});

	comp->parent = this;
	markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

/**
 *
 */
void component_t::removeChild(component_t* comp) {

	children.erase(std::remove(children.begin(), children.end(), comp), children.end());
	comp->parent = 0;
	markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

/**
 *
 */
component_t* component_t::getComponentAt(g_point p) {

	for (int i = children.size() - 1; i >= 0; i--) {
		component_t* child = children[i];

		if (child->bounds.contains(p)) {
			return child->getComponentAt(g_point(p.x - child->bounds.x, p.y - child->bounds.y));
		}
	}

	return this;
}

/**
 *
 */
window_t* component_t::getWindow() {
	window_t* thisAsWindow = dynamic_cast<window_t*>(this);
	if (thisAsWindow != 0) {
		return thisAsWindow;
	}

	if (parent) {
		return parent->getWindow();
	}

	return 0;
}

/**
 *
 */
void component_t::bringChildToFront(component_t* comp) {
	for (uint32_t index = 0; index < children.size(); index++) {
		if (children[index] == comp) {
			children.erase(children.begin() + index);
			children.push_back(comp);
			markDirty(comp->bounds);
			break;
		}
	}
}

/**
 *
 */
void component_t::bringToFront() {
	component_t* parent = getParent();
	if (parent) {
		parent->bringChildToFront(this);
	}
}

/**
 *
 */
g_point component_t::getLocationOnScreen() {
	g_point locationOnScreen(bounds.x, bounds.y);

	if (parent) {
		g_point parentLocationOnScreen = parent->getLocationOnScreen();
		locationOnScreen.x += parentLocationOnScreen.x;
		locationOnScreen.y += parentLocationOnScreen.y;
	}

	return locationOnScreen;
}

/**
 *
 */
bool component_t::handle(event_t& event) {

	locatable_t* locatable = dynamic_cast<locatable_t*>(&event);

	for (int i = children.size() - 1; i >= 0; i--) {
		component_t* child = children[i];

		if (child->visible) {
			if (locatable) {
				if (child->bounds.contains(locatable->position)) {
					locatable->position.x -= child->bounds.x;
					locatable->position.y -= child->bounds.y;

					if (child->handle(event)) {
						return true;
					}

					locatable->position.x += child->bounds.x;
					locatable->position.y += child->bounds.y;
				}

			} else if (child->handle(event)) {
				return true;
			}
		}
	}

	// post key event to client
	key_event_t* key_event = dynamic_cast<key_event_t*>(&event);
	if (key_event) {
		event_listener_info_t info;
		if (getListener(G_UI_COMPONENT_EVENT_TYPE_KEY, info)) {
			g_ui_component_key_event posted_key_event;
			posted_key_event.header.type = G_UI_COMPONENT_EVENT_TYPE_KEY;
			posted_key_event.header.component_id = info.component_id;
			posted_key_event.key_info = key_event->info;
			g_send_message(info.target_thread, &posted_key_event, sizeof(g_ui_component_key_event));
		}
	}

	// post mouse event to client
	mouse_event_t* mouse_event = dynamic_cast<mouse_event_t*>(&event);
	if (mouse_event) {
		event_listener_info_t info;
		if (getListener(G_UI_COMPONENT_EVENT_TYPE_MOUSE, info)) {
			g_ui_component_mouse_event posted_event;
			posted_event.header.type = G_UI_COMPONENT_EVENT_TYPE_MOUSE;
			posted_event.header.component_id = info.component_id;
			posted_event.position = mouse_event->position;
			posted_event.type = mouse_event->type;
			posted_event.buttons = mouse_event->buttons;
			g_send_message(info.target_thread, &posted_event, sizeof(g_ui_component_mouse_event));
		}
	}

	return false;
}

/**
 *
 */
void component_t::setPreferredSize(const g_dimension& size) {
	preferredSize = size;
}

/**
 *
 */
void component_t::setMinimumSize(const g_dimension& size) {
	minimumSize = size;
}

/**
 *
 */
void component_t::setMaximumSize(const g_dimension& size) {
	maximumSize = size;
}

/**
 *
 */
void component_t::setLayoutManager(layout_manager_t* newMgr) {
	newMgr->setComponent(this);
	this->layoutManager = newMgr;
	markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

/**
 *
 */
void component_t::layout() {
	if (layoutManager) {
		layoutManager->layout();
	}
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

/**
 *
 */
void component_t::update() {
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
void component_t::paint() {
}

/**
 *
 */
void component_t::markParentFor(component_requirement_t req) {
	if (parent) {
		parent->markFor(req);
	}
}

/**
 *
 */
void component_t::markFor(component_requirement_t req) {
	requirements |= req;

	if (parent) {
		parent->markChildsFor(req);
	}
}

/**
 *
 */
void component_t::markChildsFor(component_requirement_t req) {
	childRequirements |= req;

	if (parent) {
		parent->markChildsFor(req);
	}
}

/**
 *
 */
void component_t::resolveRequirement(component_requirement_t req) {

	if (childRequirements & req) {
		for (component_t* child : children) {
			if (child->visible) {
				child->resolveRequirement(req);
			}
		}
		childRequirements &= ~COMPONENT_REQUIREMENT_NONE;
	}

	if (requirements & req) {

		if (req == COMPONENT_REQUIREMENT_LAYOUT) {
			layout();

		} else if (req == COMPONENT_REQUIREMENT_UPDATE) {
			update();

		} else if (req == COMPONENT_REQUIREMENT_PAINT) {
			paint();

			/*
			 * TODO: This might be useful for paint-blitting
			 *
			 g_rectangle bounds = getBounds();
			 g_rectangle clip(0, 0, bounds.width, bounds.height);

			 for (Component* child : children) {
			 g_rectangle cb = child->getBounds();
			 Point loc(cb.x, cb.y);

			 child->getGraphics()->blitTo(graphics.getBuffer(), bounds, clip, loc);
			 }
			 */

			markDirty();

		}

		requirements &= ~req;
	}

}

/**
 *
 */
void component_t::setListener(g_ui_component_event_type eventType, g_tid target_thread, g_ui_component_id id) {
	event_listener_info_t info;
	info.target_thread = target_thread;
	info.component_id = id;
	listeners[eventType] = info;
}

/**
 *
 */
bool component_t::getListener(g_ui_component_event_type eventType, event_listener_info_t& out) {

	if (listeners.count(eventType)) {
		out = listeners[eventType];
		return true;
	}
	return false;
}

/**
 *
 */
void component_t::clearSurface() {
	// clear surface
	auto cr = graphics.getContext();
	cairo_save(cr);
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);
}

/**
 *
 */
bool component_t::isChildOf(component_t* c) {

	component_t* next = parent;
	do {
		if (next == c) {
			return true;
		}
		next = next->getParent();
	} while (next);

	return false;
}

/**
 *
 */
bool component_t::getNumericProperty(int property, uint32_t* out) {

	return false;
}

/**
 *
 */
bool component_t::setNumericProperty(int property, uint32_t value) {

	if (property == G_UI_PROPERTY_LAYOUT_MANAGER) {
		if (value == G_UI_LAYOUT_MANAGER_FLOW) {
			setLayoutManager(new flow_layout_manager_t());
			return true;
		} else if (value == G_UI_LAYOUT_MANAGER_GRID) {
			setLayoutManager(new grid_layout_manager_t(1, 1));
			return true;
		}
	}

	return false;
}
