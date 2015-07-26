#include <components/CheckBox.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <events/MouseEvent.hpp>

/**
 *
 */
CheckBox::CheckBox() :
		Component(true), checked(false), boxSize(DEFAULT_BOX_SIZE), boxTextGap(DEFAULT_BOX_TEXT_GAP), hovered(false), pressed(false) {
	addChild(&label);
}

/**
 *
 */
void CheckBox::layout() {
	g_dimension preferredSize = label.getPreferredSize();
	if (preferredSize.height < boxSize + boxTextGap) {
		preferredSize.height = boxSize + boxTextGap;
	}
	preferredSize.width += preferredSize.height;
	setPreferredSize(preferredSize);
}

/**
 *
 */
void CheckBox::paint() {
	g_rectangle bounds = getBounds();
	g_painter p(graphics);
	p.setColor(pressed ? RGB(240, 240, 240) : (hovered ? RGB(245, 245, 255) : RGB(255, 255, 255)));
	p.fill(g_rectangle(0, 0, boxSize, boxSize));
	p.setColor((hovered || pressed) ? RGB(140, 140, 150) : RGB(160, 160, 170));
	p.draw(g_rectangle(0, 0, boxSize - 1, boxSize - 1));

	if (checked) {
		p.setColor(RGB(70, 180, 255));
		p.fill(g_rectangle(4, 4, boxSize - 8, boxSize - 8));
	}
}

/**
 *
 */
bool CheckBox::handle(Event& e) {

	MouseEvent* me = dynamic_cast<MouseEvent*>(&e);
	if (me) {
		if (me->type == MOUSE_EVENT_ENTER) {
			hovered = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		} else if (me->type == MOUSE_EVENT_LEAVE) {
			hovered = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		} else if (me->type == MOUSE_EVENT_PRESS) {
			pressed = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		} else if (me->type == MOUSE_EVENT_RELEASE || me->type == MOUSE_EVENT_DRAG_RELEASE) {
			pressed = false;

			g_rectangle minbounds = getBounds();
			minbounds.x = 0;
			minbounds.y = 0;
			if (minbounds.contains(me->position)) {
				checked = !checked;
			}

			markFor(COMPONENT_REQUIREMENT_PAINT);
		}
		return true;
	}

	return false;
}

/**
 *
 */
void CheckBox::handleBoundChange(g_rectangle oldBounds) {
	g_rectangle unpositioned = getBounds();
	unpositioned.x = boxSize + boxTextGap;
	unpositioned.y = 0;
	this->label.setBounds(unpositioned);
}
