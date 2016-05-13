#ifndef GRIDLAYOUTMANAGER_HPP_
#define GRIDLAYOUTMANAGER_HPP_

#include <layout/layout_manager.hpp>
#include <ghostuser/graphics/metrics/insets.hpp>

/**
 *
 */
class grid_layout_manager_t: public layout_manager_t {
private:
	int columns;
	int rows;
	g_insets padding;
	int horizontalCellSpace;
	int verticalCellSpace;

public:
	grid_layout_manager_t(int columns, int rows);

	void setPadding(g_insets _padding) {
		padding = _padding;
	}

	g_insets getPadding() const {
		return padding;
	}

	void setHorizontalCellSpace(int _space) {
		horizontalCellSpace = _space;
	}

	int getHorizontalCellSpace() const {
		return horizontalCellSpace;
	}

	void setVerticalCellSpace(int _space) {
		verticalCellSpace = _space;
	}

	int getVerticalCellSpace() const {
		return verticalCellSpace;
	}

	virtual void layout();
};

#endif
