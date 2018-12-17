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

#ifndef __COMPONENT__
#define __COMPONENT__

#include <stdio.h>
#include <vector>
#include <map>
#include <events/event.hpp>
#include <layout/layout_manager.hpp>
#include <components/bounds_event_component.hpp>
#include <components/event_listener_info.hpp>
#include <ghostuser/graphics/metrics/rectangle.hpp>
#include <ghostuser/graphics/graphics.hpp>
#include <ghostuser/tasking/lock.hpp>

// forward declarations
class window_t;

/**
 * A component requirement is a flag that is put on the component
 * to mark that one specific action or multiple actions are required
 * for the component.
 *
 * For example, when a component has changed model data in an update
 * cycle, the component may also want to repaint its content, therefore
 * marks itself as paint-required.
 */
#define COMPONENT_REQUIREMENT_NONE		0
#define COMPONENT_REQUIREMENT_PAINT		1
#define COMPONENT_REQUIREMENT_LAYOUT	2
#define COMPONENT_REQUIREMENT_UPDATE	4
#define COMPONENT_REQUIREMENT_ALL		0xFFFFFFFF

typedef uint32_t component_requirement_t;

/**
 *
 */
typedef uint32_t component_child_reference_type_t;

#define COMPONENT_CHILD_REFERENCE_TYPE_DEFAULT		0
#define COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL		1

class component_t;
class component_child_reference_t {
public:
	component_t* component;
	component_child_reference_type_t type;
};

struct component_listener_entry_t {
	component_listener_entry_t* previous;
	g_ui_component_event_type type;
	event_listener_info_t info;
	component_listener_entry_t* next;
};

/**
 *
 */
class component_t: public bounds_event_component_t {
private:
	g_rectangle bounds;
	component_t* parent;
	std::vector<component_child_reference_t> children;
	g_lock children_lock;


	g_dimension minimumSize;
	g_dimension preferredSize;
	g_dimension maximumSize;

	component_requirement_t requirements;
	component_requirement_t childRequirements;

	component_listener_entry_t* listeners = 0;

	int z_index = 1000;

protected:
	layout_manager_t* layoutManager;
	g_graphics graphics;

	bool visible;

public:
	g_ui_component_id id;

	/**
	 * Creates the component; initially marks it as dirty
	 * and sets no parent
	 */
	component_t(bool transparentBackground = false) :
			id(-1), graphics(transparentBackground), visible(true), requirements(COMPONENT_REQUIREMENT_ALL), childRequirements(COMPONENT_REQUIREMENT_ALL), parent(
					0), layoutManager(0), bounds_event_component_t(this) {
	}

	/**
	 * Destroys the component
	 */
	virtual ~component_t();

	/**
	 *
	 */
	void setZIndex(int z_index) {
		this->z_index = z_index;
	}

	/**
	 * Returns a Pointer to the components graphics
	 */
	g_graphics* getGraphics() {
		return &graphics;
	}

	/**
	 * Returns the components parent
	 *
	 * @return the components parent
	 */
	component_t* getParent() {
		return parent;
	}

	/**
	 *
	 */
	std::vector<component_child_reference_t>& getChildren() {
		return children;
	}

	/**
	 *
	 */
	bool isVisible() const {
		return visible;
	}

	/**
	 *
	 */
	bool canHandleEvents() const;

	/**
	 *
	 */
	void setVisible(bool visible);

	/**
	 * Sets the bounds of the component and recreates its graphics buffer.
	 *
	 * @param bounds	the new bounds of the component
	 */
	void setBounds(const g_rectangle& bounds);

	/**
	 * Returns the bounds of the component.
	 *
	 * @return the bounds
	 */
	g_rectangle getBounds() const {
		return bounds;
	}

	/**
	 *
	 */
	void setPreferredSize(const g_dimension& size);

	/**
	 *
	 */
	virtual g_dimension getPreferredSize() {
		return preferredSize;
	}

	/**
	 *
	 */
	void setMinimumSize(const g_dimension& size);

	/**
	 *
	 */
	g_dimension getMinimumSize() const {
		return minimumSize;
	}

	/**
	 *
	 */
	void setMaximumSize(const g_dimension& size);

	/**
	 *
	 */
	g_dimension getMaximumSize() const {
		return maximumSize;
	}

	/**
	 * This method is used to blit the component and all of its children
	 * to the out buffer
	 *
	 * @param absClip	absolute bounds that may not be exceeded
	 * @param position	absolute screen position to blit to
	 */
	void blit(g_graphics* out, g_rectangle absClip, g_point position);

	/**
	 * Adds the given component as a child to this component
	 *
	 * @param comp	the component to add
	 */
	virtual void addChild(component_t* comp, component_child_reference_type_t type = COMPONENT_CHILD_REFERENCE_TYPE_DEFAULT);

	/**
	 * Removes the given component from this component
	 *
	 * @param comp	the component to remove
	 */
	void removeChild(component_t* comp);

	/**
	 * Returns the component at the given Point
	 *
	 * @param p		the Point
	 */
	virtual component_t* getComponentAt(g_point p);

	/**
	 * Returns the first in the hierarchy that is a Window
	 *
	 * @return the window
	 */
	virtual window_t* getWindow();

	/**
	 * @return whether this type of component is a window.
	 */
	virtual bool isWindow() {
		return false;
	}

	/**
	 * Brings this component to the front
	 */
	virtual void bringToFront();

	/**
	 * Brings the given child component to the front
	 *
	 * @param comp	the child component
	 */
	virtual void bringChildToFront(component_t* comp);

	/**
	 * Returns the location of the component on screen
	 *
	 * @return the location
	 */
	virtual g_point getLocationOnScreen();

	/**
	 * Handles the given event
	 *
	 * @return true if it was handled, otherwise false
	 */
	virtual bool handle(event_t& event);

	/**
	 *
	 */
	virtual void handleBoundChange(g_rectangle oldBounds) {
		// May be implemented by subtypes
	}

	/**
	 *
	 */
	virtual void setLayoutManager(layout_manager_t* layoutManager);

	/**
	 *
	 */
	layout_manager_t* getLayoutManager() const {
		return layoutManager;
	}

	/**
	 * Marks the given area as dirty so it is copied to the framebuffer
	 *
	 * @param rect	the g_rectangle to mark dirty
	 */
	virtual void markDirty(g_rectangle rect);

	/**
	 * Marks the entire component as dirty
	 */
	virtual void markDirty() {
		markDirty(g_rectangle(0, 0, bounds.width, bounds.height));
	}

	/**
	 * Places the flag for the given requirement on the parent component (if non-null).
	 */
	void markParentFor(component_requirement_t req);

	/**
	 * Places the flag for the given requirement on this component.
	 */
	void markFor(component_requirement_t req);

	/**
	 * Places the flag for the given requirement in the list of child-requirements.
	 */
	void markChildsFor(component_requirement_t req);

	/**
	 * Resolves the given requirement
	 */
	void resolveRequirement(component_requirement_t req);

	/**
	 * This method is called by the window manager if the layout requirement flag is set.
	 * The component may here change the bounds of each child component and also change its
	 * own preferred size.
	 */
	virtual void layout();

	/**
	 * This method is called by the window manager if the update requirement flag is set.
	 * The component may here change the contents of it's model.
	 */
	virtual void update();

	/**
	 * This method is called by the window manager if the paint requirement flag is set.
	 * The component may here repaint itself.
	 */
	virtual void paint();

	/**
	 *
	 */
	virtual bool getNumericProperty(int property, uint32_t* out);

	/**
	 *
	 */
	virtual bool setNumericProperty(int property, uint32_t value);

	/**
	 *
	 */
	void setListener(g_ui_component_event_type eventType, g_tid target_thread, g_ui_component_id id);

	/**
	 *
	 */
	bool getListener(g_ui_component_event_type eventType, event_listener_info_t& out);

	/**
	 *
	 */
	void clearSurface();

	/**
	 *
	 */
	bool isChildOf(component_t* c);

	/**
	 * Returns the reference to the given component (if the given component is a child of this component).
	 *
	 * @return true if the component was found
	 */
	bool getChildReference(component_t* child, component_child_reference_t& out);

};

#endif
