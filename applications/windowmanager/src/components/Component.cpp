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

#include <components/Component.hpp>
#include <components/Window.hpp>
#include <events/Locatable.hpp>
#include <WindowManager.hpp>

#include <ghostuser/graphics/painter.hpp>
#include <ghostuser/utils/Logger.hpp>
#include <algorithm>

/**
 *
 */
Component::~Component() {
	if (layoutManager) {
		delete layoutManager;
	}
}

/**
 *
 */
void Component::setBounds(const g_rectangle& newBounds) {
	g_rectangle oldBounds = bounds;

	markDirty();
	bounds = newBounds;
	markDirty();

	if (oldBounds.width != newBounds.width || oldBounds.height != newBounds.height) {
		graphics.resize(newBounds.width, newBounds.height);
		markFor(COMPONENT_REQUIREMENT_LAYOUT);
		markFor(COMPONENT_REQUIREMENT_UPDATE);
	}

	handleBoundChange(oldBounds);
}

/**
 *
 */
bool Component::canHandleEvents() const {

	if(!visible) {
		return false;
	}

	if(parent) {
		return parent->canHandleEvents();
	}

	return true;
}

/**
 *
 */
void Component::setVisible(bool visible) {
	this->visible = visible;
	markDirty();
	markFor(COMPONENT_REQUIREMENT_ALL);
}

/**
 *
 */
void Component::markDirty(g_rectangle rect) {

	if (parent) {
		rect.x += bounds.x;
		rect.y += bounds.y;
		parent->markDirty(rect);
	}

}

/**
 *
 */
void Component::blit(g_color_argb* out, g_rectangle& outBounds, g_rectangle absoluteClip, g_point offset) {

	if (this->visible) {
		if (graphics.getBuffer() != 0) {
			graphics.blitTo(out, outBounds, absoluteClip, offset);
		}

		g_rectangle thisBounds = getBounds();
		thisBounds.x += absoluteClip.x;
		thisBounds.y += absoluteClip.y;
		int newTop = thisBounds.getTop() < absoluteClip.getTop() ? absoluteClip.getTop() : thisBounds.getTop();
		int newBottom = thisBounds.getBottom() > absoluteClip.getBottom() ? absoluteClip.getBottom() : thisBounds.getBottom();
		int newLeft = thisBounds.getLeft() < absoluteClip.getLeft() ? absoluteClip.getLeft() : thisBounds.getLeft();
		int newRight = thisBounds.getRight() > absoluteClip.getRight() ? absoluteClip.getRight() : thisBounds.getRight();

		g_rectangle subClip = g_rectangle(newLeft, newTop, newRight - newLeft, newBottom - newTop);
		for (Component* c : children) {
			c->blit(out, outBounds, subClip, g_point(offset.x + c->bounds.x, offset.y + c->bounds.y));
		}
	}
}

/**
 *
 */
void Component::addChild(Component* comp) {

	if (comp->parent) {
		comp->parent->removeChild(comp);
	}

	children.push_back(comp);
	comp->parent = this;
	markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

/**
 *
 */
void Component::removeChild(Component* comp) {

	children.erase(std::remove(children.begin(), children.end(), comp), children.end());
	comp->parent = 0;
	markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

/**
 *
 */
Component* Component::getComponentAt(g_point p) {

	for (int i = children.size() - 1; i >= 0; i--) {
		Component* child = children[i];

		if (child->bounds.contains(p)) {
			return child->getComponentAt(g_point(p.x - child->bounds.x, p.y - child->bounds.y));
		}
	}

	return this;
}

/**
 *
 */
Window* Component::getWindow() {
	Window* thisAsWindow = dynamic_cast<Window*>(this);
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
void Component::bringChildToFront(Component* comp) {
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
void Component::bringToFront() {
	Component* parent = getParent();
	if (parent) {
		parent->bringChildToFront(this);
	}
}

/**
 *
 */
g_point Component::getLocationOnScreen() {
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
bool Component::handle(Event& event) {

	Locatable* locatable = dynamic_cast<Locatable*>(&event);

	for (int i = children.size() - 1; i >= 0; i--) {
		Component* child = children[i];

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

	return false;
}

/**
 *
 */
void Component::setPreferredSize(const g_dimension& size) {
	preferredSize = size;
}

/**
 *
 */
void Component::setMinimumSize(const g_dimension& size) {
	minimumSize = size;
}

/**
 *
 */
void Component::setMaximumSize(const g_dimension& size) {
	maximumSize = size;
}

/**
 *
 */
void Component::setLayoutManager(LayoutManager* newMgr) {
	newMgr->setComponent(this);
	this->layoutManager = newMgr;
	markFor(COMPONENT_REQUIREMENT_LAYOUT);
}

/**
 *
 */
void Component::layout() {
	if (layoutManager) {
		layoutManager->layout();
	}
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

/**
 *
 */
void Component::update() {
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
void Component::paint() {
}

/**
 *
 */
void Component::markParentFor(ComponentRequirement req) {
	if (parent) {
		parent->markFor(req);
	}
}

/**
 *
 */
void Component::markFor(ComponentRequirement req) {
	requirements |= req;

	if (parent) {
		parent->markChildsFor(req);
	}

	WindowManager::getInstance()->markForRender();
}

/**
 *
 */
void Component::markChildsFor(ComponentRequirement req) {
	childRequirements |= req;

	if (parent) {
		parent->markChildsFor(req);
	}
}

/**
 *
 */
void Component::resolveRequirement(ComponentRequirement req) {

	if (childRequirements & req) {
		for (Component* child : children) {
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
