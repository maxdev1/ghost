#include <components/component.hpp>
#include <vector>

#include <ghostuser/utils/logger.hpp>
#include <layout/grid_layout_manager.hpp>

#include <typeinfo>

/**
 *
 */
grid_layout_manager_t::grid_layout_manager_t(int columns, int rows) :
		columns(columns), rows(rows), padding(g_insets(0, 0, 0, 0)), horizontalCellSpace(0), verticalCellSpace(0) {
}

/**
 *
 */
void grid_layout_manager_t::layout() {

	if (component == 0) {
		return;
	}

	std::vector<component_child_reference_t>& children = component->getChildren();

	g_rectangle usedBounds = component->getBounds();
	usedBounds.x = padding.left;
	usedBounds.y = padding.top;
	usedBounds.width -= padding.left + padding.right;
	usedBounds.height -= padding.top + padding.bottom;

	int x = usedBounds.x;
	int y = usedBounds.y;
	int rowHeight = 0;

	int widthPerComponent = (columns > 0) ? (usedBounds.width / columns) : usedBounds.width;

	for (auto& ref : children) {
		component_t* c = ref.component;

		int usedHeight = (rows > 0) ? (usedBounds.height / rows) : c->getPreferredSize().height;

		if (x + widthPerComponent > usedBounds.width) {
			x = usedBounds.x;
			y += rowHeight;
			rowHeight = 0;
		}

		c->setBounds(g_rectangle(x, y, widthPerComponent, usedHeight));
		x += widthPerComponent;

		if (usedHeight > rowHeight) {
			rowHeight = usedHeight;
		}

	}
}
