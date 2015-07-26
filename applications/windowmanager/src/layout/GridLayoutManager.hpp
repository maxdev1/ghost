#ifndef GRIDLAYOUTMANAGER_HPP_
#define GRIDLAYOUTMANAGER_HPP_

#include <layout/LayoutManager.hpp>

/**
 *
 */
class GridLayoutManager: public LayoutManager {
private:
	int columns;
	int rows;
public:
	GridLayoutManager(int columns, int rows);

	virtual void layout();
};

#endif
