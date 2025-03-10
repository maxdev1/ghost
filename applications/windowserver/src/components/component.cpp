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
#include "component_registry.hpp"
#include "windowserver.hpp"

#include <cairo/cairo.h>
#include <libwindow/properties.hpp>
#include <libwindow/interface.hpp>
#include <algorithm>
#include <stdlib.h>
#include <typeinfo>
#include <layout/flex_layout_manager.hpp>

component_t::component_t() :
	bounding_component_t(this),
	focusable_component_t(this),
	visible(true),
	requirements(COMPONENT_REQUIREMENT_ALL),
	childRequirements(COMPONENT_REQUIREMENT_ALL),
	parent(nullptr),
	layoutManager(nullptr)
{
	id = component_registry_t::add(this);
}

component_t::~component_t()
{
	delete layoutManager;
}

void component_t::setBoundsInternal(const g_rectangle& newBounds)
{
	g_mutex_acquire(lock);

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
		if(hasGraphics())
		{
			graphics.resize(bounds.width, bounds.height);
		}
		markFor(COMPONENT_REQUIREMENT_ALL);
		handleBoundChanged(oldBounds);
	}

	g_mutex_release(lock);
}

void component_t::recheckGraphics()
{
	if(hasGraphics())
		graphics.resize(bounds.width, bounds.height);
}

g_rectangle component_t::getBounds() const
{
	g_mutex_acquire(lock);
	auto bounds = this->bounds;
	g_mutex_release(lock);
	return bounds;
}

std::vector<component_child_reference_t>& component_t::acquireChildren()
{
	g_mutex_acquire(childrenLock);
	return children;
}

void component_t::releaseChildren() const
{
	g_mutex_release(childrenLock);
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
	g_mutex_acquire(lock);

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

	g_mutex_release(lock);

	this->callForListeners(G_UI_COMPONENT_EVENT_TYPE_VISIBLE, [visible](event_listener_info_t& info)
	{
		g_ui_component_visible_event e;
		e.header.type = G_UI_COMPONENT_EVENT_TYPE_VISIBLE;
		e.header.component_id = info.component_id;
		e.visible = visible;
		g_send_message(info.target_thread, &e, sizeof(g_ui_component_visible_event));
	});
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

void component_t::blit(graphics_t* out, const g_rectangle& parentClip, const g_point& screenPosition)
{
	if(!this->visible)
		return;

	g_mutex_acquire(lock);

	g_rectangle clip = getBounds();
	clip.x = screenPosition.x;
	clip.y = screenPosition.y;
	clip = clip.clip(parentClip);

	if(hasGraphics())
	{
		graphics.blitTo(out, clip, screenPosition);
	}
	g_mutex_release(lock);

	this->blitChildren(out, clip, screenPosition);
}

void component_t::blitChildren(graphics_t* out, const g_rectangle& clip, const g_point& screenPosition)
{
	g_mutex_acquire(childrenLock);
	for(auto& c: children)
	{
		if(c.component->visible)
		{
			g_point childPositionOnParent = screenPosition + c.component->bounds.getStart();
			c.component->blit(out, clip, childPositionOnParent);
		}
	}
	g_mutex_release(childrenLock);
}

void component_t::addChild(component_t* comp, component_child_reference_type_t type)
{
	if(comp->parent)
		comp->parent->removeChild(comp);

	comp->parent = this;

	component_child_reference_t reference;
	reference.component = comp;
	reference.type = type;

	g_mutex_acquire(childrenLock);
	children.push_back(reference);
	std::stable_sort(children.begin(), children.end(),
	                 [](const component_child_reference_t& c1, const component_child_reference_t& c2)
	                 {
		                 return c1.component->zIndex < c2.component->zIndex;
	                 });
	g_mutex_release(childrenLock);

	comp->markFor(COMPONENT_REQUIREMENT_ALL);
	markFor(COMPONENT_REQUIREMENT_ALL);
}

void component_t::removeChild(component_t* comp)
{
	comp->parent = 0;

	g_mutex_acquire(childrenLock);
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
	g_mutex_release(childrenLock);

	markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

component_t* component_t::getComponentAt(g_point p)
{
	component_t* target = this;

	g_mutex_acquire(childrenLock);
	for(auto it = children.rbegin(); it != children.rend(); ++it)
	{
		auto child = (*it).component;
		if(child->visible && child->bounds.contains(p))
		{
			g_mutex_release(childrenLock);
			target = child->getComponentAt(g_point(p.x - child->bounds.x, p.y - child->bounds.y));
			break;
		}
	}
	g_mutex_release(childrenLock);

	return target;
}

window_t* component_t::getWindow()
{
	if(isWindow())
		return dynamic_cast<window_t*>(this);

	if(parent)
		return parent->getWindow();

	return nullptr;
}

void component_t::bringChildToFront(component_t* comp)
{
	g_mutex_acquire(childrenLock);
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
	g_mutex_release(childrenLock);
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

	g_mutex_acquire(childrenLock);
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
	g_mutex_release(childrenLock);

	if(handledByChild)
		return handledByChild;

	auto handledByListener = this->callForListeners(G_UI_COMPONENT_EVENT_TYPE_MOUSE,
	                                                [event](event_listener_info_t& info)
	                                                {
		                                                g_ui_component_mouse_event postedEvent;
		                                                postedEvent.header.type = G_UI_COMPONENT_EVENT_TYPE_MOUSE;
		                                                postedEvent.header.component_id = info.component_id;
		                                                postedEvent.position = event.position;
		                                                postedEvent.type = event.type;
		                                                postedEvent.buttons = event.buttons;
		                                                postedEvent.clickCount = event.clickCount;
	                                                	postedEvent.scroll = event.scroll;
		                                                g_send_message(
				                                                info.target_thread, &postedEvent,
				                                                sizeof(g_ui_component_mouse_event));
	                                                });

	// TODO Temporary fix so scroll-events still go to parents
	if(event.type == G_MOUSE_EVENT_SCROLL)
		return nullptr;

	if(handledByListener)
		return this;

	return nullptr;
}

component_t* component_t::handleKeyEvent(key_event_t& event)
{
	component_t* handledByChild = nullptr;

	g_mutex_acquire(childrenLock);
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
	g_mutex_release(childrenLock);

	if(!handledByChild)
	{
		if(sendKeyEventToListener(event))
			return this;
	}

	return handledByChild;
}

bool component_t::sendKeyEventToListener(key_event_t& event)
{
	return this->callForListeners(G_UI_COMPONENT_EVENT_TYPE_KEY,
	                              [event](event_listener_info_t& info)
	                              {
		                              g_ui_component_key_event posted_key_event;
		                              posted_key_event.header.type =
				                              G_UI_COMPONENT_EVENT_TYPE_KEY;
		                              posted_key_event.header.component_id = info.component_id;
		                              posted_key_event.key_info = event.info;
		                              g_send_message(
				                              info.target_thread, &posted_key_event,
				                              sizeof(g_ui_component_key_event));
	                              });
}

void component_t::setPreferredSize(const g_dimension& size)
{
	if(preferredSize != size)
	{
		preferredSize = size;
		markParentFor(COMPONENT_REQUIREMENT_LAYOUT);
	}
}

g_dimension component_t::getEffectivePreferredSize()
{
	auto preferred = getPreferredSize();
	auto min = getMinimumSize();
	preferred.width = std::max(preferred.width, min.width);
	preferred.height = std::max(preferred.height, min.height);
	return preferred;
}

void component_t::setMinimumSize(const g_dimension& size)
{
	minimumSize = size;
	markParentFor(COMPONENT_REQUIREMENT_LAYOUT);
}

void component_t::setMaximumSize(const g_dimension& size)
{
	maximumSize = size;
	markParentFor(COMPONENT_REQUIREMENT_LAYOUT);
}

void component_t::setLayoutManager(layout_manager_t* newMgr)
{
	newMgr->setComponent(this);
	this->layoutManager = newMgr;
	markFor(COMPONENT_REQUIREMENT_LAYOUT);
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

	windowserver_t::instance()->requestUpdateLater();
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
void component_t::resolveRequirement(component_requirement_t req, int lvl)
{
	if((childRequirements & req) && !(req == COMPONENT_REQUIREMENT_LAYOUT))
	{
		g_mutex_acquire(childrenLock);
		for(auto& child: children)
		{
			if(child.component->visible)
			{
				g_mutex_acquire(child.component->lock);
				child.component->resolveRequirement(req, lvl + 1);
				g_mutex_release(child.component->lock);
			}
		}
		childRequirements &= ~req;
		g_mutex_release(childrenLock);
	}

	g_mutex_acquire(lock);
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
	g_mutex_release(lock);

	if((childRequirements & req) && req == COMPONENT_REQUIREMENT_LAYOUT)
	{
		g_mutex_acquire(childrenLock);
		for(auto& child: children)
		{
			if(child.component->visible)
			{
				g_mutex_acquire(child.component->lock);
				child.component->resolveRequirement(req, lvl + 1);
				g_mutex_release(child.component->lock);
			}
		}
		childRequirements &= ~req;
		g_mutex_release(childrenLock);
	}
}

void component_t::addListener(g_ui_component_event_type eventType, g_tid target_thread, g_ui_component_id id)
{
	g_mutex_acquire(lock);

	auto entry = new component_listener_entry_t();
	entry->info.target_thread = target_thread;
	entry->info.component_id = id;
	entry->type = eventType;
	listeners.push_back(entry);

	g_mutex_release(lock);
}

bool component_t::callForListeners(g_ui_component_event_type eventType,
                                   const std::function<void(event_listener_info_t&)>& func)
{
	g_mutex_acquire(lock);

	bool handled = false;
	for(auto& entry: listeners)
	{
		if(entry->type == eventType)
		{
			func(entry->info);
			handled = true;
		}
	}

	g_mutex_release(lock);
	return handled;
}

void component_t::clearSurface()
{
	auto cr = graphics.acquireContext();
	if(!cr)
		return;

	cairo_save(cr);
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);

	graphics.releaseContext();
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
	if(property == G_UI_PROPERTY_VISIBLE)
	{
		*out = this->isVisible() ? 1 : 0;
		return true;
	}
	else if(property == G_UI_PROPERTY_FLEX_GAP)
	{
		auto flexManager = dynamic_cast<flex_layout_manager_t*>(getLayoutManager());
		if(flexManager)
		{
			*out = flexManager->getGap();
			return true;
		}
		return false;
	}


	if(focusable_component_t::getNumericProperty(property, out))
	{
		return true;
	}
	return false;
}

bool component_t::setNumericProperty(int property, uint32_t value)
{
	if(property == G_UI_PROPERTY_LAYOUT_MANAGER)
	{
		if(value == G_UI_LAYOUT_MANAGER_FLOW)
		{
			setLayoutManager(new flow_layout_manager_t());
			return true;
		}
		if(value == G_UI_LAYOUT_MANAGER_GRID)
		{
			setLayoutManager(new grid_layout_manager_t());
			return true;
		}
		if(value == G_UI_LAYOUT_MANAGER_FLEX)
		{
			setLayoutManager(new flex_layout_manager_t());
			return true;
		}
	}
	else if(property == G_UI_PROPERTY_VISIBLE)
	{
		setVisible(value == 1);
		return true;
	}
	else if(property == G_UI_PROPERTY_FLEX_GAP)
	{
		auto flexManager = dynamic_cast<flex_layout_manager_t*>(getLayoutManager());
		if(flexManager)
		{
			flexManager->setGap(value);
			return true;
		}
		return true;
	}

	if(focusable_component_t::setNumericProperty(property, value))
	{
		return true;
	}
	return false;
}

bool component_t::setStringProperty(int property, std::string text)
{
	return false;
}

bool component_t::getStringProperty(int property, std::string& out)
{
	return false;
}

bool component_t::getChildReference(component_t* child, component_child_reference_t& out)
{
	g_mutex_acquire(childrenLock);
	for(auto& ref: children)
	{
		if(ref.component == child)
		{
			out = ref;
			g_mutex_release(childrenLock);
			return true;
		}
	}
	g_mutex_release(childrenLock);
	return false;
}
