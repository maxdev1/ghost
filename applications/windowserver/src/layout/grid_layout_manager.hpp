#ifndef GRIDLAYOUTMANAGER_HPP_
#define GRIDLAYOUTMANAGER_HPP_

#include <layout/layout_manager.hpp>

/**
 *
 */
class grid_layout_manager_t: public layout_manager_t {
private:
	int columns;
	int rows;
public:
	grid_layout_manager_t(int columns, int rows);

	virtual void layout();
};

#endif
