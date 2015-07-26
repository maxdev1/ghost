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

#ifndef COMPONENT_HPP_
#define COMPONENT_HPP_

#include <vector>
#include <ghostuser/graphics/graphics.hpp>
#include <ghostuser/graphics/metrics/rectangle.hpp>
#include <events/Event.hpp>
#include <layout/LayoutManager.hpp>

class Window;

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
typedef uint32_t ComponentRequirement;

/**
 *
 */
class Component {
private:
	g_rectangle bounds;
	Component* parent;
	std::vector<Component*> children;

	g_dimension minimumSize;
	g_dimension preferredSize;
	g_dimension maximumSize;

	ComponentRequirement requirements;
	ComponentRequirement childRequirements;

protected:
	LayoutManager* layoutManager;
	g_graphics graphics;

	bool visible;

public:
	/**
	 * Creates the component; initially marks it as dirty
	 * and sets no parent
	 */
	Component(bool transparentBackground = false) :
			graphics(transparentBackground), visible(true), requirements(COMPONENT_REQUIREMENT_ALL), childRequirements(COMPONENT_REQUIREMENT_ALL), parent(0), layoutManager(
					0) {
	}

	/**
	 * Destroys the component
	 */
	virtual ~Component();

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
	Component* getParent() {
		return parent;
	}

	/**
	 *
	 */
	std::vector<Component*>& getChildren() {
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
	 * to a big buffer, the out buffer
	 *
	 * @param out		the output buffer
	 * @param outBounds	bounds of the output buffer
	 * @param absoluteClip the absolute bounds on the buffer that may not be exceeded
	 * @param pos		offset to blit to
	 */
	void blit(g_color_argb* out, g_rectangle& outBounds, g_rectangle absoluteClip, g_point pos);

	/**
	 * Adds the given component as a child to this component
	 *
	 * @param comp	the component to add
	 */
	void addChild(Component* comp);

	/**
	 * Removes the given component from this component
	 *
	 * @param comp	the component to remove
	 */
	void removeChild(Component* comp);

	/**
	 * Returns the component at the given Point
	 *
	 * @param p		the Point
	 */
	virtual Component* getComponentAt(g_point p);

	/**
	 * Returns the first in the hierarchy that is a Window
	 *
	 * @return the window
	 */
	virtual Window* getWindow();

	/**
	 * Brings this component to the front
	 */
	virtual void bringToFront();

	/**
	 * Brings the given child component to the front
	 *
	 * @param comp	the child component
	 */
	virtual void bringChildToFront(Component* comp);

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
	virtual bool handle(Event& event);

	/**
	 *
	 */
	virtual void handleBoundChange(g_rectangle oldBounds) {
		// May be implemented by subtypes
	}

	/**
	 *
	 */
	void setLayoutManager(LayoutManager* layoutManager);

	/**
	 *
	 */
	LayoutManager* getLayoutManager() const {
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
	void markParentFor(ComponentRequirement req);

	/**
	 * Places the flag for the given requirement on this component.
	 */
	void markFor(ComponentRequirement req);

	/**
	 * Places the flag for the given requirement in the list of child-requirements.
	 */
	void markChildsFor(ComponentRequirement req);

	/**
	 * Resolves the given requirement
	 */
	void resolveRequirement(ComponentRequirement req);

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

};

#endif
