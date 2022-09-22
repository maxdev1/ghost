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

#include "components/component.hpp"
#include "components/window.hpp"
#include "events/key_event.hpp"
#include "events/locatable.hpp"
#include "events/mouse_event.hpp"
#include "layout/flow_layout_manager.hpp"
#include "layout/grid_layout_manager.hpp"
#include "windowserver.hpp"

#include <algorithm>
#include <cairo/cairo.h>

#include <typeinfo>

component_t::~component_t()
{
    if(layoutManager)
        delete layoutManager;
}

void component_t::setBounds(const g_rectangle& newBounds)
{
    g_rectangle oldBounds = bounds;
    markDirty();

    bounds = newBounds;
    if(bounds.width < minimumSize.width)
        bounds.width = minimumSize.width;
    if(bounds.height < minimumSize.height)
        bounds.height = minimumSize.height;
    markDirty();

    if(oldBounds.width != bounds.width || oldBounds.height != bounds.height)
    {
        if(needsGraphics)
            graphics.resize(bounds.width, bounds.height);
        markFor(COMPONENT_REQUIREMENT_ALL);

        handleBoundChange(oldBounds);
    }

    fireBoundsChange(bounds);
}

bool component_t::canHandleEvents() const
{
    if(!visible)
        return false;
    if(parent)
        return parent->canHandleEvents();
    return true;
}

void component_t::setVisible(bool visible)
{
    this->visible = visible;
    markDirty();

    if(visible)
    {
        markFor(COMPONENT_REQUIREMENT_ALL);
    }
    else
    {
        markParentFor(COMPONENT_REQUIREMENT_LAYOUT);
    }
}

void component_t::markDirty(g_rectangle rect)
{
    if(parent)
    {
        rect.x += bounds.x;
        rect.y += bounds.y;
        parent->markDirty(rect);
    }
}

void component_t::blit(g_graphics* out, g_rectangle clip, g_point position)
{
    if(!this->visible)
        return;

    if(graphics.getContext())
        graphics.blitTo(out, clip, position);

    g_rectangle abs = getBounds();
    abs.x = position.x;
    abs.y = position.y;

    int newTop = clip.getTop() > abs.getTop() ? clip.getTop() : abs.getTop();
    int newBottom = clip.getBottom() < abs.getBottom() ? clip.getBottom() : abs.getBottom();
    int newLeft = clip.getLeft() > abs.getLeft() ? clip.getLeft() : abs.getLeft();
    int newRight = clip.getRight() < abs.getRight() ? clip.getRight() : abs.getRight();
    g_rectangle thisClip = g_rectangle(newLeft, newTop, newRight - newLeft, newBottom - newTop);

    g_atomic_lock(&children_lock);
    for(auto& c : children)
    {
        if(c.component->visible)
            c.component->blit(out, thisClip, g_point(position.x + c.component->bounds.x, position.y + c.component->bounds.y));
    }
    children_lock = 0;
}

void component_t::addChild(component_t* comp, component_child_reference_type_t type)
{
    if(comp->parent)
        comp->parent->removeChild(comp);

    comp->parent = this;

    component_child_reference_t reference;
    reference.component = comp;
    reference.type = type;

    g_atomic_lock(&children_lock);
    children.push_back(reference);
    std::sort(children.begin(), children.end(), [](component_child_reference_t& c1, component_child_reference_t& c2)
              { return c1.component->z_index < c2.component->z_index; });
    children_lock = 0;

    markFor(COMPONENT_REQUIREMENT_ALL);
}

void component_t::removeChild(component_t* comp)
{
    comp->parent = 0;

    g_atomic_lock(&children_lock);
    for(auto itr = children.begin(); itr != children.end();)
    {
        if((*itr).component == comp)
        {
            itr = children.erase(itr);
        }
        else
        {
            ++itr;
        }
    }
    children_lock = 0;

    markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

component_t* component_t::getComponentAt(g_point p)
{
    component_t* target = this;

    g_atomic_lock(&children_lock);
    for(auto it = children.rbegin(); it != children.rend(); ++it)
    {
        auto child = (*it).component;
        if(child->visible && child->bounds.contains(p))
        {
            children_lock = 0;
            target = child->getComponentAt(g_point(p.x - child->bounds.x, p.y - child->bounds.y));
            break;
        }
    }
    children_lock = 0;

    return target;
}

window_t* component_t::getWindow()
{
    if(isWindow())
        return (window_t*) this;

    if(parent)
        return parent->getWindow();

    return 0;
}

void component_t::bringChildToFront(component_t* comp)
{
    g_atomic_lock(&children_lock);
    for(uint32_t index = 0; index < children.size(); index++)
    {
        if(children[index].component == comp)
        {
            auto ref = children[index];
            children.erase(children.begin() + index);
            children.push_back(ref);

            markDirty(comp->bounds);
            break;
        }
    }
    children_lock = 0;
}

void component_t::bringToFront()
{
    if(parent)
        parent->bringChildToFront(this);
}

g_point component_t::getLocationOnScreen()
{
    g_point location(bounds.x, bounds.y);

    if(parent)
        location += parent->getLocationOnScreen();

    return location;
}

component_t* component_t::handleMouseEvent(mouse_event_t& event)
{
    component_t* handledByChild = nullptr;

    g_atomic_lock(&children_lock);
    for(auto it = children.rbegin(); it != children.rend(); ++it)
    {
        auto child = (*it).component;
        if(!child->visible)
            continue;

        if(child->bounds.contains(event.position))
        {
            event.position.x -= child->bounds.x;
            event.position.y -= child->bounds.y;

            handledByChild = child->handleMouseEvent(event);
            if(handledByChild)
            {
                break;
            }

            event.position.x += child->bounds.x;
            event.position.y += child->bounds.y;
        }
    }
    children_lock = 0;

    return handledByChild;
}

component_t* component_t::handleKeyEvent(key_event_t& event)
{
    component_t* handledByChild = nullptr;

    g_atomic_lock(&children_lock);
    for(auto it = children.rbegin(); it != children.rend(); ++it)
    {
        auto child = (*it).component;
        if(!child->visible)
            continue;

        handledByChild = child->handleKeyEvent(event);
        if(handledByChild)
        {
            break;
        }
    }
    children_lock = 0;

    return handledByChild;
}

component_t* component_t::handleFocusEvent(focus_event_t& event)
{
    component_t* handledByChild = nullptr;

    g_atomic_lock(&children_lock);
    for(auto it = children.rbegin(); it != children.rend(); ++it)
    {
        auto child = (*it).component;
        if(!child->visible)
            continue;

        handledByChild = child->handleFocusEvent(event);
        if(handledByChild)
        {
            break;
        }
    }
    children_lock = 0;

    return handledByChild;
}

void component_t::setPreferredSize(const g_dimension& size)
{
    preferredSize = size;
}

void component_t::setMinimumSize(const g_dimension& size)
{
    minimumSize = size;
}

void component_t::setMaximumSize(const g_dimension& size)
{
    maximumSize = size;
}

void component_t::setLayoutManager(layout_manager_t* newMgr)
{
    newMgr->setComponent(this);
    this->layoutManager = newMgr;
    markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

void component_t::update()
{
    markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

void component_t::layout()
{
    if(layoutManager)
    {
        layoutManager->layout();
        markFor(COMPONENT_REQUIREMENT_PAINT);
    }
}

void component_t::paint()
{
}

void component_t::markParentFor(component_requirement_t req)
{
    if(parent)
        parent->markFor(req);
}

void component_t::markFor(component_requirement_t req)
{
    requirements |= req;

    if(parent)
        parent->markChildsFor(req);
}

void component_t::markChildsFor(component_requirement_t req)
{
    childRequirements |= req;

    if(parent)
        parent->markChildsFor(req);
}

/**
 * Resolves a single requirement in the component tree. Layouting is done top-down,
 * while updating and painting is done bottom-up.
 */
void component_t::resolveRequirement(component_requirement_t req)
{
    if((childRequirements & req) && req != COMPONENT_REQUIREMENT_LAYOUT)
    {
        g_atomic_lock(&children_lock);
        for(auto& child : children)
        {
            if(child.component->visible)
                child.component->resolveRequirement(req);
        }
        childRequirements &= ~req;
        children_lock = 0;
    }

    if(requirements & req)
    {
        if(req == COMPONENT_REQUIREMENT_UPDATE)
        {
            update();
        }
        else if(req == COMPONENT_REQUIREMENT_LAYOUT)
        {
            layout();
        }
        else if(req == COMPONENT_REQUIREMENT_PAINT)
        {
            paint();
            markDirty();
        }

        requirements &= ~req;
    }

    if((childRequirements & req) && req == COMPONENT_REQUIREMENT_LAYOUT)
    {
        g_atomic_lock(&children_lock);
        for(auto& child : children)
        {
            if(child.component->visible)
                child.component->resolveRequirement(req);
        }
        childRequirements &= ~req;
        children_lock = 0;
    }
}

void component_t::setListener(g_ui_component_event_type eventType, g_tid target_thread, g_ui_component_id id)
{
    component_listener_entry_t* entry = new component_listener_entry_t();
    entry->info.target_thread = target_thread;
    entry->info.component_id = id;
    entry->previous = 0;
    entry->type = eventType;

    if(listeners)
    {
        entry->next = listeners;
        listeners->previous = entry;
    }
    else
    {
        entry->next = 0;
    }
    listeners = entry;
}

bool component_t::getListener(g_ui_component_event_type eventType, event_listener_info_t& out)
{

    component_listener_entry_t* entry = listeners;
    while(entry)
    {
        if(entry->type == eventType)
        {
            out = entry->info;
            return true;
        }
        entry = entry->next;
    }
    return false;
}

void component_t::clearSurface()
{
    auto cr = graphics.getContext();
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_restore(cr);
}

bool component_t::isChildOf(component_t* component)
{
    component_t* next = parent;
    while(next)
    {
        if(next == component)
        {
            return true;
        }
        next = next->getParent();
    }

    return false;
}

bool component_t::getNumericProperty(int property, uint32_t* out)
{
    return false;
}

bool component_t::setNumericProperty(int property, uint32_t value)
{
    return false;
}

bool component_t::getChildReference(component_t* child, component_child_reference_t& out)
{

    g_atomic_lock(&children_lock);
    for(auto& ref : children)
    {
        if(ref.component == child)
        {
            out = ref;
            children_lock = 0;
            return true;
        }
    }
    children_lock = 0;
    return false;
}
