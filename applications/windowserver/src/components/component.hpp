/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __WINDOWSERVER_COMPONENTS_COMPONENT__
#define __WINDOWSERVER_COMPONENTS_COMPONENT__

#include "components/bounding_component.hpp"
#include "components/event_listener_info.hpp"
#include "focusable_component.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"
#include "layout/layout_manager.hpp"
#include "video/graphics.hpp"

#include <libwindow/interface.hpp>
#include <libwindow/metrics/rectangle.hpp>
#include <vector>
#include <bits/std_function.h>


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
#define COMPONENT_REQUIREMENT_NONE 0
#define COMPONENT_REQUIREMENT_PAINT 1
#define COMPONENT_REQUIREMENT_LAYOUT 2
#define COMPONENT_REQUIREMENT_UPDATE 4
#define COMPONENT_REQUIREMENT_ALL 0xFFFFFFFF

typedef uint32_t component_requirement_t;

/**
 * Type used to differentiate how a child component relates to the parent
 * component. If it is an internal component that may not be deleted
 * separately, it is referenced with the internal type.
 */
typedef uint32_t component_child_reference_type_t;

#define COMPONENT_CHILD_REFERENCE_TYPE_DEFAULT 0
#define COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL 1

class component_t;

class component_child_reference_t
{
public:
    component_t* component;
    component_child_reference_type_t type;
};

struct component_listener_entry_t
{
    g_ui_component_event_type type;
    event_listener_info_t info;
};

/**
 *
 */
class component_t : public bounding_component_t, public focusable_component_t
{
protected:
    g_user_mutex lock = g_mutex_initialize_r(true);
    g_rectangle bounds;
    component_t* parent;
    std::vector<component_child_reference_t> children;
    g_user_mutex childrenLock = g_mutex_initialize_r(true);

    g_dimension minimumSize;
    g_dimension preferredSize;
    g_dimension maximumSize;

    component_requirement_t requirements;
    component_requirement_t childRequirements;

    std::vector<component_listener_entry_t*> listeners;

    int zIndex = 1000;

    layout_manager_t* layoutManager;
    graphics_t graphics;

    bool visible;

    /**
     * If a component that extends this class does not need to paint anything, it should return false here.
     */
    virtual bool hasGraphics() const
    {
        return true;
    }

public:
    g_ui_component_id id;

    component_t();
    ~component_t() override;

    bool isVisible() const
    {
        return this->visible;
    }

    /**
     * Sets the z-index of this component on its parent. When new children are added,
     * all children are sorted based on this z-index.
     */
    void setZIndex(int zIndex)
    {
        this->zIndex = zIndex;
    }

    /**
     * Returns the components parent
     *
     * @return the components parent
     */
    component_t* getParent()
    {
        return parent;
    }

    std::vector<component_child_reference_t>& acquireChildren();

    void releaseChildren() const;

    bool canHandleEvents() const;

    void setVisible(bool visible);

    /**
     * Sets the bounds of the component and recreates its graphics buffer.
     *
     * @param bounds	the new bounds of the component
     */
    void setBoundsInternal(const g_rectangle& bounds) override;

    /**
     * Returns the bounds of the component.
     *
     * @return the bounds
     */
    g_rectangle getBounds() const;

    void setPreferredSize(const g_dimension& size);

    virtual g_dimension getPreferredSize()
    {
        return preferredSize;
    }

    virtual g_dimension getEffectivePreferredSize();

    void setMinimumSize(const g_dimension& size);

    g_dimension getMinimumSize() const
    {
        return minimumSize;
    }

    void setMaximumSize(const g_dimension& size);

    g_dimension getMaximumSize() const
    {
        return maximumSize;
    }

    /**
     * This method is used to blit the component and all of its children
     * to the out buffer
     *
     * @param clip	absolute bounds that may not be exceeded
     * @param screenPosition	absolute screen position to blit to
     */
    virtual void blit(graphics_t* out, const g_rectangle& clip, const g_point& screenPosition);

    virtual void blitChildren(graphics_t* out, const g_rectangle& clip, const g_point& screenPosition);

    /**
     * Adds the given component as a child to this component
     *
     * @param comp	the component to add
     */
    virtual void addChild(component_t* comp,
                          component_child_reference_type_t type = COMPONENT_CHILD_REFERENCE_TYPE_DEFAULT);

    /**
     * Removes the given component from this component
     *
     * @param comp	the component to remove
     */
    virtual void removeChild(component_t* comp);

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
    virtual bool isWindow()
    {
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

    virtual component_t* handleMouseEvent(mouse_event_t& event);
    virtual component_t* handleKeyEvent(key_event_t& event);

    virtual void handleBoundChanged(const g_rectangle& oldBounds)
    {
        // May be implemented by subtypes
    }

    virtual void setLayoutManager(layout_manager_t* layoutManager);

    virtual layout_manager_t* getLayoutManager() const
    {
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
    virtual void markDirty()
    {
        markDirty(g_rectangle(0, 0, bounds.width + 1, bounds.height + 1));
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
    void resolveRequirement(component_requirement_t req, int lvl);

    bool hasChildRequirements() const
    {
        return childRequirements != COMPONENT_REQUIREMENT_NONE;
    }

    bool isChildOf(component_t* c);

    /**
     * Returns the reference to the given component (if the given component is a child of this component).
     *
     * @return true if the component was found
     */
    bool getChildReference(component_t* child, component_child_reference_t& out);

    /**
     * This method is called by the window manager if the update requirement flag is set.
     * The component may here change the contents of it's model.
     */
    virtual void update();

    /**
     * This method is called by the window manager if the layout requirement flag is set.
     * The component may here change the bounds of each child component and also change its
     * own preferred size.
     */
    virtual void layout();

    /**
     * This method is called by the window manager if the paint requirement flag is set.
     * The component may here repaint itself.
     */
    virtual void paint();

    void clearSurface();

    virtual bool getNumericProperty(int property, uint32_t* out);
    virtual bool setNumericProperty(int property, uint32_t value);

    void addListener(g_ui_component_event_type eventType, g_tid target_thread, g_ui_component_id id);
    bool callForListeners(g_ui_component_event_type eventType, const std::function<void(event_listener_info_t&)>& func);
};

#endif
