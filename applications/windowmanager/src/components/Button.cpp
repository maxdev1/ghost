#include <components/Button.hpp>
#include <components/Window.hpp>
#include <events/MouseEvent.hpp>
#include <events/FocusEvent.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <ghostuser/graphics/text/text_alignment.hpp>
#include <ghostuser/utils/Logger.hpp>

/**
 *
 */
Button::Button() :
		insets(g_insets(5, 5, 5, 5)) {
	addChild(&label);
	label.setAlignment(g_text_alignment::CENTER);
}

/**
 *
 */
void Button::layout() {
	g_dimension labelPreferred = label.getPreferredSize();

	labelPreferred.width += insets.left + insets.right;
	labelPreferred.height += insets.top + insets.bottom;
	setPreferredSize(labelPreferred);
}

/**
 *
 */
void Button::paint() {

	g_painter p(graphics);
	p.setColor(state.pressed ? RGB(230, 230, 230) : (state.hovered ? RGB(250, 250, 250) : RGB(240, 240, 240)));
	p.fill(g_rectangle(0, 0, getBounds().width, getBounds().height));

	if (state.focused) {
		p.setColor(RGB(55, 155, 255));
	} else {
		p.setColor(RGB(180, 180, 180));
	}
	p.draw(g_rectangle(0, 0, getBounds().width - 1, getBounds().height - 1));
}

/**
 *
 */
bool Button::handle(Event& e) {

	MouseEvent* me = dynamic_cast<MouseEvent*>(&e);
	if (me) {
		if (me->type == MOUSE_EVENT_ENTER) {
			state.hovered = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		} else if (me->type == MOUSE_EVENT_LEAVE) {
			state.hovered = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		} else if (me->type == MOUSE_EVENT_PRESS) {
			state.pressed = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		} else if (me->type == MOUSE_EVENT_RELEASE || me->type == MOUSE_EVENT_DRAG_RELEASE) {
			state.pressed = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);

			if (me->type == MOUSE_EVENT_RELEASE) {
				if (me->position.x >= 0 && me->position.y >= 0 && me->position.x < getBounds().width && me->position.y < getBounds().height) {
					fireAction();
				}
			}
		}
		return true;
	}

	FocusEvent* fe = dynamic_cast<FocusEvent*>(&e);
	if (fe) {
		if (fe->type == FOCUS_EVENT_GAINED) {
			state.focused = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			return true;
		} else if (fe->type == FOCUS_EVENT_LOST) {
			state.focused = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			return true;
		}
	}

	return false;
}

/**
 *
 */
void Button::handleBoundChange(g_rectangle oldBounds) {
	g_rectangle labelBounds = getBounds();
	labelBounds.x = insets.left;
	labelBounds.y = insets.right;
	this->label.setBounds(labelBounds);
}

/**
 *
 */
void Button::setTitle(std::string title) {
	this->label.setTitle(title);
}

/**
 *
 */
std::string Button::getTitle() {
	return this->label.getTitle();
}
